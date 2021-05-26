let exports = {__esModule: true}

async function require(module) {

    const src = module.replace("./", "/src/")

    const script = document.createElement("script")
    script.type = "module"
    script.src = src
    document.head.appendChild(script)
    let loaded = false
    await new Promise(resolve => script.addEventListener('load', () => {
        console.log("loaded")
        resolve()
    }))
    return exports
}

