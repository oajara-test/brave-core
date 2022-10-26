// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {I18nMixin, I18nMixinInterface} from 'chrome://resources/js/i18n_mixin.js'
import {PrefsMixin, PrefsMixinInterface} from '../prefs/prefs_mixin.js'
import '../settings_shared.css.js'
import '../settings_vars.css.js'
import {getTemplate} from './bookmark_bar.html.js'

const SettingsBraveAppearanceBookmarkBarElementBase = PrefsMixin(I18nMixin(PolymerElement)) as {
  new (): PolymerElement & I18nMixinInterface & PrefsMixinInterface
}

enum BookmarkBarState {
  ALWAYS = 0,
  NONE = 1,
  NTP = 2,
}

/**
 * 'settings-brave-appearance-bookmark-bar' is the settings page area containing
 * brave's bookmark bar visibility settings in appearance settings.
 */
export class SettingsBraveAppearanceBookmarkBarElement extends SettingsBraveAppearanceBookmarkBarElementBase {
  static get is() {
    return 'settings-brave-appearance-bookmark-bar'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      /** @private {chrome.settingsPrivate.PrefType} */
      bookmarkBarStatePref_: {
        key: '',
        type: Object,
        value() {
          return {
            key: '',
            type: chrome.settingsPrivate.PrefType.NUMBER,
            value: BookmarkBarState.NTP
          }
        }
      }
    }
  }

  bookmarkBarStatePref_: chrome.settingsPrivate.PrefObject

  private bookmarkBarShowOptions_ = [
    {value: BookmarkBarState.ALWAYS, name: this.i18n('appearanceSettingsBookmarBarAlways')},
    {value: BookmarkBarState.NONE, name: this.i18n('appearanceSettingsBookmarBarNever')},
    {value: BookmarkBarState.NTP, name: this.i18n('appearanceSettingsBookmarBarNTP')}
  ]
  private bookmarkBarShowEnabledLabel_: string

  static get observers() {
    return [
      'onPrefsChanged_(prefs.bookmark_bar.show_on_all_tabs.value, prefs.brave.always_show_bookmark_bar_on_ntp.value)'
    ]
  }

  override ready() {
    super.ready()
    window.addEventListener('load', this.onLoad_.bind(this));
  }
  /** @private **/
  onLoad_() {
    this.setControlValueFromPrefs()
  }

  private getBookmarkBarStateFromPrefs(): BookmarkBarState {
    if (this.getPref('bookmark_bar.show_on_all_tabs').value)
      return BookmarkBarState.ALWAYS

    if (this.getPref('brave.always_show_bookmark_bar_on_ntp').value)
      return BookmarkBarState.NTP
    return BookmarkBarState.NONE
  }

  private saveBookmarkBarStateToPrefs(state: BookmarkBarState) {
    if (state === BookmarkBarState.ALWAYS) {
      this.setPrefValue('bookmark_bar.show_on_all_tabs', true)
    } else if (state === BookmarkBarState.NTP) {
      this.setPrefValue('bookmark_bar.show_on_all_tabs', false)
      this.setPrefValue('brave.always_show_bookmark_bar_on_ntp', true)
    } else {
      this.setPrefValue('bookmark_bar.show_on_all_tabs', false)
      this.setPrefValue('brave.always_show_bookmark_bar_on_ntp', false)
    }
  }
  private setControlValueFromPrefs() {
    const state = this.getBookmarkBarStateFromPrefs()
    if (this.bookmarkBarStatePref_.value === state)
      return
    this.bookmarkBarStatePref_ = {
      key: '',
      type: chrome.settingsPrivate.PrefType.NUMBER,
      value: state
    };
  }
  private onPrefsChanged_() {
    this.setControlValueFromPrefs()
  }
  private onShowOptionChanged_() {
    const state = this.bookmarkBarStatePref_.value
    if (state === BookmarkBarState.ALWAYS) {
      this.bookmarkBarShowEnabledLabel_ = this.i18n('appearanceSettingsBookmarBarAlwaysDesc')
    } else if (state === BookmarkBarState.NTP) {
      this.bookmarkBarShowEnabledLabel_ = this.i18n('appearanceSettingsBookmarBarNTPDesc')
    } else {
      this.bookmarkBarShowEnabledLabel_ = this.i18n('appearanceSettingsBookmarBarNeverDesc')
    }

    this.saveBookmarkBarStateToPrefs(this.bookmarkBarStatePref_.value)
  }

}

customElements.define(SettingsBraveAppearanceBookmarkBarElement.is, SettingsBraveAppearanceBookmarkBarElement)
