var spawn = require('child-process-promise').spawn
const conf = require('../eosioConfig')
const fs = require('fs-extra')

/**
 * @param {string} exec native exec
 * @param {string[]} params parameters to pass to executable
 */
async function runCommand(exec, params) {

  var promise = spawn(exec, params, { cwd: '../' })

  var childProcess = promise.childProcess

  console.log('[spawn] childProcess.pid: ', childProcess.pid)

  childProcess.stdout.on('data', function (data) {
    // console.log('stdout: ', data.toString())
  })
  childProcess.stderr.on('data', function (data) {
    console.log('stderr: ', data.toString())
  })

  return promise
}

const includes = [
  './include'
]

const initPrams = (chain) => {
  let params = [`./src/${conf.cppName}.cpp`, '-abigen']
  includes.forEach(el => {
    params = params.concat(['-I', el])
  })
  params = params.concat(['-O', '3'])
  return params
}

const methods = {
  async debug(data) {
    try {
      // fs.ensureDirSync('../build/debug')
      const params = initPrams('debug')
        .concat(['-o', `./build/${conf.contractName}.wasm`])
      console.log("Building with params:")
      console.log(params)
      await runCommand('eosio-cpp', params)

    } catch (error) {
      console.error(error.toString())
    }

  },
  async prod() {

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