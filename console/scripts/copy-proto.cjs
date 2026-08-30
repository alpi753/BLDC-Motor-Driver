const { copyFileSync, mkdirSync } = require("node:fs")
const { join } = require("node:path")

const source = join(__dirname, "../../protocol/bldc.proto")
const destinationDirectory = join(__dirname, "../dist-electron/protocol")

mkdirSync(destinationDirectory, { recursive: true })
copyFileSync(source, join(destinationDirectory, "bldc.proto"))
