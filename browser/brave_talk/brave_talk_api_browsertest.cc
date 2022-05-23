#include <memory>
#include <string>
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/test_timeouts.h"
#include "brave/browser/brave_talk/brave_talk_media_access_handler.h"
#include "brave/browser/brave_talk/brave_talk_service.h"
#include "brave/browser/brave_talk/brave_talk_service_factory.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include "brave/browser/brave_talk/brave_talk_tab_capture_registry_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"
#include "ubidiimp.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

class BraveTalkAPIBrowserTest : public InProcessBrowserTest {
 public:
  BraveTalkAPIBrowserTest()
      : http_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUp() override {
    http_server_.AddDefaultHandlers(
        base::FilePath(FILE_PATH_LITERAL("brave/test/data")));

    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    ASSERT_NE(talk_service(), nullptr);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(http_server_.Start());

    // By default, all SSL cert checks are valid. Can be overridden in tests.
    cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);

    SetRequesterFrameOrigins("talk.brave.com", "talk.brave.com");

    NavigateParams launch_tab(
        browser(), http_server_.GetURL("example.com", "/brave_talk/test.html"),
        ui::PAGE_TRANSITION_LINK);
    launch_tab.disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
    ui_test_utils::NavigateToURL(&launch_tab);

    ASSERT_EQ(2, browser()->tab_strip_model()->count());
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Browser will both run and display insecure content.
    command_line->AppendSwitch(switches::kAllowRunningInsecureContent);
    cert_verifier_.SetUpCommandLine(command_line);
  }

 protected:
  void NavigateToURLAndWait(const GURL& url) {
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    content::TestNavigationObserver observer(
        web_contents, content::MessageLoopRunner::QuitMode::DEFERRED);
    NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
    ui_test_utils::NavigateToURL(&params);
    observer.WaitForNavigationFinished();
  }

  void SetRequesterFrameOrigins(const std::string& main_frame_origin, const std::string& sub_frame_origin) {
    const GURL& origin = http_server_.GetURL(sub_frame_origin, "/brave_talk/frame.html");
    auto root_url = http_server_.GetURL(
        main_frame_origin, "/brave_talk/test.html?sub_frame=" + origin.spec());
    NavigateToURLAndWait(root_url);
  }

  content::WebContents* requester_contents() {
    return browser()->tab_strip_model()->GetWebContentsAt(0);
  }

  content::RenderFrameHost* requester_main_frame() {
    return requester_contents()->GetMainFrame();
  }

  content::RenderFrameHost* requester_sub_frame() {
    auto all_frames = content::CollectAllRenderFrameHosts(
        requester_contents()->GetPrimaryPage());
    EXPECT_EQ(2u, all_frames.size());
    for (auto* frame : all_frames) {
      if (frame != requester_main_frame())
        return frame;
    }
    CHECK(false);
    return nullptr;
  }

  content::WebContents* target_contents() {
    return browser()->tab_strip_model()->GetWebContentsAt(1);
  }

  brave_talk::BraveTalkService* talk_service() {
    return brave_talk::BraveTalkServiceFactory::GetForContext(
        browser()->profile());
  }

  brave_talk::BraveTalkTabCaptureRegistry* registry() {
    return brave_talk::BraveTalkTabCaptureRegistryFactory::GetForContext(
        browser()->profile());
  }

  net::EmbeddedTestServer* http_server() { return &http_server_; }

 private:
  net::EmbeddedTestServer http_server_;
  content::ContentMockCertVerifier cert_verifier_;
  GURL brave_talk_url_ = GURL("https://talk.brave.com/");
};

IN_PROC_BROWSER_TEST_F(BraveTalkAPIBrowserTest, CanRequestCapture) {
  ASSERT_TRUE(content::ExecJs(requester_contents(), "requestCapture()"));
  talk_service()->ShareTab(target_contents());

  auto result = content::EvalJs(requester_contents(), "window.deviceIdPromise");
  EXPECT_FALSE(nullptr == result);
  EXPECT_NE("", result);

  // We should have a share request for the |target_contents()| now.
  EXPECT_TRUE(registry()->VerifyRequest(
      target_contents()->GetMainFrame()->GetProcess()->GetID(),
      target_contents()->GetMainFrame()->GetRoutingID()));

  EXPECT_EQ(true,
            content::EvalJs(requester_contents(),
                            "startCapture('" + result.ExtractString() + "');"));
}

IN_PROC_BROWSER_TEST_F(BraveTalkAPIBrowserTest,
                       CanRequestCaptureForSubframeOnSameOrigin) {
  ASSERT_TRUE(content::ExecJs(requester_contents(), "requestCapture(frame)"));
  talk_service()->ShareTab(target_contents());

  auto result = content::EvalJs(requester_contents(), "window.deviceIdPromise");
  EXPECT_FALSE(nullptr == result);
  EXPECT_NE("", result);

  // We should have a share request for the |target_contents()| now.
  EXPECT_TRUE(registry()->VerifyRequest(
      target_contents()->GetMainFrame()->GetProcess()->GetID(),
      target_contents()->GetMainFrame()->GetRoutingID()));

  EXPECT_TRUE(content::ExecJs(
      requester_contents(),
      "delegateCaptureToFrame('" + result.ExtractString() + "');"));
  EXPECT_EQ(true, content::EvalJs(requester_sub_frame(),
                                  "window.startCapturePromise"));
}

IN_PROC_BROWSER_TEST_F(BraveTalkAPIBrowserTest,
                       CanRequestCaptureForSubframeOnDifferentOrigin) {
  SetRequesterFrameOrigins("talk.brave.com", "example.com");
  ASSERT_TRUE(content::ExecJs(requester_contents(), "requestCapture(frame)"));
  talk_service()->ShareTab(target_contents());

  auto result = content::EvalJs(requester_contents(), "window.deviceIdPromise");
  EXPECT_FALSE(nullptr == result);
  EXPECT_NE("", result);

  // We should have a share request for the |target_contents()| now.
  EXPECT_TRUE(registry()->VerifyRequest(
      target_contents()->GetMainFrame()->GetProcess()->GetID(),
      target_contents()->GetMainFrame()->GetRoutingID()));

  EXPECT_TRUE(content::ExecJs(
      requester_contents(),
      "delegateCaptureToFrame('" + result.ExtractString() + "');"));
  EXPECT_EQ(true, content::EvalJs(requester_sub_frame(),
                                  "window.startCapturePromise"));
                                  base::RunLoop().Run();
}

IN_PROC_BROWSER_TEST_F(BraveTalkAPIBrowserTest, DISABLED_NavigationClearsShareRequest) {
  std::string device_id;
  talk_service()->GetDeviceID(
      requester_contents(),
      requester_contents()->GetMainFrame()->GetProcess()->GetID(),
      requester_contents()->GetMainFrame()->GetRoutingID(),
      base::BindLambdaForTesting(
          [&device_id](const std::string& result) { device_id = result; }));
  EXPECT_TRUE(talk_service()->is_requesting_tab());

  // Navigate, same origin.
  NavigateToURLAndWait(GURL("https://talk.brave.com/foo"));

  EXPECT_FALSE(talk_service()->is_requesting_tab());
  EXPECT_EQ("", device_id);

  talk_service()->GetDeviceID(
      requester_contents(),
      requester_contents()->GetMainFrame()->GetProcess()->GetID(),
      requester_contents()->GetMainFrame()->GetRoutingID(),
      base::BindLambdaForTesting(
          [&device_id](const std::string& result) { device_id = result; }));
  EXPECT_TRUE(talk_service()->is_requesting_tab());

  // Navigate, new origin.
  NavigateToURLAndWait(GURL("https://foo.bar"));

  EXPECT_FALSE(talk_service()->is_requesting_tab());
  EXPECT_EQ("", device_id);
}
