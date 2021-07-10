
const conf = require('../eosioConfig')
const env = require('../.env.js')
const { api, tapos, doAction } = require('./lib/eosjs')()

const methods = {

  async fullReset(channel) {
    doAction('clrtokens', { channel })
    doAction('clrreporters', { channel })
    doAction('clrtransfers', { channel })
    doAction('clrreports', { channel })
    doAction('clrchannels', {})
    doAction('clrsettings', {})
    await doAction('clear.exp', { channel, count: 100 })
  }

}


if (require.main == module) {
  if (Object.keys(methods).find(el => el === process.argv[2])) {
    console.log("Starting:", process.argv[2])
    methods[process.argv[2]](...process.argv.slice(3)).catch((error) => console.error(error))
      .then((result) => console.log('Finished'))
  } else {
    console.log("Available Commands:")
    console.log(JSON.stringify(Object.keys(methods), null, 2))
  }
}
module.exports = methods