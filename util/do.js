
const conf = require('../eosioConfig')
const env = require('../.env.js')
const { api, tapos, doAction } = require('./lib/eosjs')()
const activeChain = process.env.CHAIN || env.defaultChain

const methods = {
  async claimpoints(reporter) {
    await doAction('claimpoints', {
      reporter
    })
  },
  async claimfees(reporter, token) {
    await doAction('claimfees', {
      reporter, token
    })
  },
  async enable(enable) {
    enable = JSON.parse(enable.toLowerCase());
    await doAction('enable', {
      enable
    })
  },
  async init() {
    await doAction('init', {
      admin_account: conf.accountName[activeChain],
      current_chain_name: activeChain.toLowerCase(),
      expire_after_seconds: 600,
      weight_threshold: 2
    })
  },
  async addchannel(name, remote) {
    await doAction('addchannel', {
      channel_name: name,
      remote_contract: remote
    })
  },
  async addtoken1() {
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
      min_quantity: "0.5000 TLOS",
      fee_pct: 1,
      fee_flat: "0.0001 TLOS"
    })
  },
  async addtoken2() {
    await doAction('addtoken', {
      channel: 'wax',
      do_issue: false,
      enabled: true,
      remote_token: {
        contract: 'ibcbridgetkn',
        sym: '4,TLOS'
      },
      token_symbol: {
        contract: 'eosio.token',
        sym: '4,TLOS'
      },
      min_quantity: "0.5000 TLOS",
      fee_pct: 1,
      fee_flat: "0.0001 TLOS"
    })
  },
  async addreporter(reporter, weight) {
    await doAction('addreporter', {
      reporter,
      weight: parseInt(weight)
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