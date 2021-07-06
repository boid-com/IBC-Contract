
const conf = require('../eosioConfig')
const env = require('../.env.js')
const { api, tapos, doAction } = require('./lib/eosjs')()
const activeChain = process.env.CHAIN || env.defaultChain

const methods = {
  async init() {
    await doAction('init', {
      admin_account: conf.accountName[activeChain],
      current_chain_name: activeChain.toLowerCase(),
      expire_after_seconds: 600,
      weight_threshold: 2
    })
  },
  async addtoken() {
    await doAction('addtoken', {
      channel: 'telos',
      do_issue: true,
      enabled: true,
      remote_token: {
        contract: 'eosio.token',
        sym: '4,TLOS'
      },
      token_symbol: {
        contract: 'ibcbridgetkn',
        sym: '4,TLOS'
      },
      min_quantity: "1.0000 TLOS"
    })
  },
  async rmtoken() {
    await doAction('rmtoken', {})
  },

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