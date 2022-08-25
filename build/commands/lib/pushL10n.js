const path = require('path')
const config = require('../lib/config')
const util = require('../lib/util')
const l10nUtil = require('./l10nUtil')

const pushL10n = (options) => {
  const runOptions = { cwd: config.srcDir }
  const cmdOptions = config.defaultOptions
  cmdOptions.cwd = config.braveCoreDir
  const extraScriptOptions =
    options.with_translations ? '--with_translations' :
      options.with_missing_translations ? '--with_missing_translations' : ''

  if (options.extension) {
    const getMessages = l10nUtil.extensionScriptPaths[options.extension]
    if (!getMessages) {
      console.error('Unknown extension: ', options.extension, 'Valid extensions are: ', Object.keys(extensionScriptPaths).join(", "))
      process.exit(1)
    }

    getMessages(options.extension_path).forEach((sourceStringPath) => {
      util.run('python', ['script/push-l10n.py', '--source_string_path', sourceStringPath], cmdOptions)
    })
  }

  // Get rid of local copied xtb and grd changes
  let args = ['checkout', '--', '*.xtb']
  util.run('git', args, runOptions)
  args = ['checkout', '--', '*.grd*']
  util.run('git', args, runOptions)
  l10nUtil.getBraveTopLevelPaths().forEach((sourceStringPath) => {
    if (!options.grd_path || sourceStringPath.endsWith(path.sep + options.grd_path))
      util.run('python3', ['script/push-l10n.py', '--source_string_path', sourceStringPath, extraScriptOptions], cmdOptions)
  })
}

module.exports = pushL10n
