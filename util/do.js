
const conf = require('../eosioConfig')
const env = require('../.env.js')
const { api, tapos, doAction } = require('./lib/eosjs')()

const methods = {
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
  async report() {
    doAction('report', {
      reporter: 'ibcbridgedev',
      channel: 'telos',
      transfer: {
        id: 1,
        transaction_id: '79F169E4822E13B904FB395A6940C46B53F618233568663645BCF6C813F77912',
        from_blockchain: 'telos',
        to_blockchain: 'wax',
        from_account: 'eosio.token',
        to_account: 'eosio.token',
        quantity: '100.0000 TLOS',
        memo: 'test',
        transaction_time: new Date(),
        expires_at: new Date(Date.now() + 6000000),
        is_refund: false
      }

    })
  },
  // async addtoken

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