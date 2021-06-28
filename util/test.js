
const conf = require('../eosioConfig')
const env = require('../.env.js')
const { api, tapos, doAction } = require('./lib/eosjs')()

const methods = {

  async fullReset() {
    doAction('clrtokens', { channel: "wax" })
    doAction('clrreporters', { channel: "wax" })
    doAction('clrtransfers', { channel: "wax" })
    doAction('clrreports', { channel: "wax" })
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