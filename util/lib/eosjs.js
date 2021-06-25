const { JsonRpc, Api } = require('eosjs')
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig')
const { TextDecoder, TextEncoder } = require('util')
const tapos = {
  blocksBehind: 20,
  expireSeconds: 30
}
let api
let rpc

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
    console.log(`https://${chainName()}.bloks.io/transaction/` + txid)
    // console.log(txid)
    return result
  } catch (error) {
    console.error(error.toString())
    if (error.json) console.error("Logs:", error.json?.error?.details[1]?.message)
  }
}
function init(keys, apiurl) {
  if (!keys) keys = []
  const signatureProvider = new JsSignatureProvider(keys)
  const fetch = require('node-fetch')
  if (!apiurl) apiurl = 'http://localhost:3051'
  rpc = new JsonRpc(apiurl, { fetch })
  api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() })

  return { api, rpc, tapos, doAction }
}

module.exports = init