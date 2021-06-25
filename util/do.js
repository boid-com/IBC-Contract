const conf = require('../eosioConfig')
const env = require('../.env.js')
const { api, tapos, doAction } = require('./lib/eosjs')(env.keys[env.defaultChain], conf.endpoints[env.defaultChain][0])
const contractAccount = conf.accountName[env.defaultChain]
var watchAccountSample = require('./lib/sample_watchaccount')
function chainName() {
  if (env.defaultChain == 'jungle') return 'jungle3'
  else return env.defaultChain
}



const methods = {
  // async addsignevent(contract, max_deposit) {
  //   await doAction('whitelisttkn', { tknwhitelist: { contract, max_deposit } })
  // },

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