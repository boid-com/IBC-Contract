const fs = require(`fs`)
const path = require(`path`)
const crypto = require(`crypto`)
const { Serialize, Api } = require(`eosjs`)
const { TextEncoder, TextDecoder } = require(`util`)

const loadFileContents = file => {
  if (!fs.existsSync(file)) {
    throw new Error(`Could not retrieve code file "${path.resolve(file)}".`)
  }
  return fs.readFileSync(file)
}

const jsonToRawAbi = json => {
  const tmpApi = new Api({
    textDecoder: new TextDecoder(),
    textEncoder: new TextEncoder(),
  })
  const buffer = new Serialize.SerialBuffer({
    textEncoder: tmpApi.textEncoder,
    textDecoder: tmpApi.textDecoder,
  })

  const abiDefinition = tmpApi.abiTypes.get(`abi_def`)

  // need to make sure abi has every field in abiDefinition.fields
  // otherwise serialize throws
  const jsonExtended = abiDefinition.fields.reduce(
    (acc, { name: fieldName }) => Object.assign(acc, { [fieldName]: acc[fieldName] || [] }),
    json,
  )

  // fs.writeFileSync('./abiMod.abi', JSON.stringify(jsonExtended, null, 2))
  abiDefinition.serialize(buffer, jsonExtended)
  // console.log('here');
  if (!Serialize.supportedAbiVersion(buffer.getString())) {
    throw new Error(`Unsupported abi version`)
  }
  buffer.restartRead()

  // convert to node buffer
  return Buffer.from(buffer.asUint8Array())
}

function setCodeAction(wasmFile, authorization) {

  const contents = loadFileContents(wasmFile)

  const wasm = contents.toString(`hex`)
  const result = {
    account: `eosio`,
    name: `setcode`,
    authorization,
    data: {
      account: authorization[0].actor,
      vmtype: 0,
      vmversion: 0,
      code: wasm,
    },
  }
  // console.log(result)
  return result
}

function setAbiAction(abiFile, authorization) {
  const contents = loadFileContents(abiFile)

  let abi
  try {
    abi = JSON.parse(contents.toString(`utf8`))
  } catch (error) {
    throw new Error(
      `Cannot parse contents of ABI file ${path.resolve(account.abi)}:\n\t${error.message}`,
    )
  }
  // console.log('setting abi');

  const serializedAbi = jsonToRawAbi(abi).toString(`hex`)
  // console.log('setting abi');

  const result = {
    account: `eosio`,
    name: `setabi`,
    authorization,
    data: { account: authorization[0].actor, abi: serializedAbi },
  }
  // console.log(result);
  return result
}

module.exports = { setAbiAction, setCodeAction }