require('dotenv').config()
const { JsonRpc, Api } = require('eosjs')
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig')
const { TextDecoder, TextEncoder } = require('util')
const conf = require('../../eosioConfig')
const env = require('../../.env.js')
const activeChain = process.env.CHAIN || env.defaultChain
console.log('Active Chain:', activeChain)
const contractAccount = conf.accountName[activeChain]
const tapos = {
  blocksBehind: 20,
  expireSeconds: 30
}
let api
let rpc

const formatBloksTransaction = (network, txId) => {
  let bloksSubdomain = `bloks.io`
  switch (network) {
    case `jungle3`:
      bloksSubdomain = `jungle3.bloks.io`;
      break;
    case `telosTest`:
      bloksSubdomain = `telos-test.bloks.io`;
      break;
    case `waxTest`:
      bloksSubdomain = `wax-test.bloks.io`;
      break;
    case `eos`:
      bloksSubdomain = `bloks.io`;
      break;
    case `wax`:
      bloksSubdomain = `wax.bloks.io`;
      break;
    case `telos`:
      bloksSubdomain = `telos.bloks.io`;
      break;
  }
  return `https://${bloksSubdomain}/transaction/${txId}`;
};

async function doAction(name, data, account, auth) {
  try {
    if (!data) data = {}
    if (!account) account = contractAccount
    if (!auth) auth = account
    console.log("Do Action:", name, data)
    const authorization = [{ actor: auth, permission: 'active' }]
    const result = await api.transact({
      // "delay_sec": 0,
      actions: [{ account, name, data, authorization }]
    }, tapos)
    const txid = result.transaction_id
    // console.log(`https://${chainName()}.bloks.io/transaction/` + txid)
    console.log(formatBloksTransaction(activeChain, txid))
    return result
  } catch (error) {
    console.error(error.toString())
    if (error.json) console.error("Logs:", error.json?.error?.details[1]?.message)
  }
}
function init(keys, apiurl) {
  if (!keys) keys = env.keys[activeChain]
  const signatureProvider = new JsSignatureProvider(keys)
  const fetch = require('node-fetch')

  if (!apiurl) apiurl = conf.endpoints[activeChain][0]
  rpc = new JsonRpc(apiurl, { fetch })
  api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() })

  return { api, rpc, tapos, doAction, formatBloksTransaction }
}

module.exports = init