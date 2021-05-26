import {x} from "./other";
import {y} from "./third";

setTimeout(() => {
    const button = document.createElement("button")
    button.appendChild(document.createTextNode("Click Me"))

    button.addEventListener('click', () => {
        pre.innerHTML = `${pre.innerHTML}
${x} ${y}`
    })

    const pre = document.createElement("pre")
    document.body.appendChild(button)
    document.body.appendChild(pre)

}, 5);