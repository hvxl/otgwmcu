const otainfo = fetch("otainfo.json").then(r => r.json()).then(data => {
    return data
})

window.addEventListener("load", async () => {
    let info = await otainfo
    for (let name in info) {
        let w = document.getElementById(name)
        if (w) {
            w.innerText = info[name]
        }
    }
})
