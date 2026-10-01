"use strict";

const codeEl   = document.getElementById("code");
const outputEl = document.getElementById("output");
const runBtn   = document.getElementById("run");
const clearBtn = document.getElementById("clear");
const clearOut = document.getElementById("clearOut");
const statusEl = document.getElementById("status");
const examples = document.getElementById("examples");

const SAMPLES = {
    hello: `-- hello.sgas
func greet(name) {
    print("Hello, " + name + "!");
}

greet("SGas");
greet("World");
`,
    fib: `-- fib.sgas
func fib(n) {
    if n < 2 {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

let i = 0;
while i < 10 {
    print("fib(" + str(i) + ") = " + str(fib(i)));
    i = i + 1;
}
`,
    loop: `-- حلقه و شرط
let i = 1;
while i <= 5 {
    if i % 2 == 0 {
        print(str(i) + " زوج است");
    } else {
        print(str(i) + " فرد است");
    }
    i = i + 1;
}
`,
    empty: ""
};

function setStatus(text, cls) {
    statusEl.textContent = text;
    statusEl.className = "status" + (cls ? " " + cls : "");
}

function showOutput(text, isError) {
    outputEl.innerHTML = "";
    const span = document.createElement("span");
    if (isError) span.className = "err";
    span.textContent = text;
    outputEl.appendChild(span);
}

async function runCode() {
    const code = codeEl.value;
    if (!code.trim()) {
        showOutput("(کد خالی است)", false);
        return;
    }

    runBtn.disabled = true;
    setStatus("در حال اجرا...");
    showOutput("");

    try {
        const res = await fetch("/run", {
            method: "POST",
            headers: { "Content-Type": "text/plain; charset=utf-8" },
            body: code
        });

        const text = await res.text();
        showOutput(text || "(بدون خروجی)", !res.ok);
        setStatus(res.ok ? "اجرا موفق" : "خطا", res.ok ? "ok" : "err");
    } catch (e) {
        showOutput("خطای شبکه: " + e.message, true);
        setStatus("خطای شبکه", "err");
    } finally {
        runBtn.disabled = false;
    }
}

runBtn.addEventListener("click", runCode);

codeEl.addEventListener("keydown", (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === "Enter") {
        e.preventDefault();
        runCode();
    }
    // Tab = 4 spaces
    if (e.key === "Tab") {
        e.preventDefault();
        const s = codeEl.selectionStart;
        const en = codeEl.selectionEnd;
        codeEl.value = codeEl.value.substring(0, s) + "    " + codeEl.value.substring(en);
        codeEl.selectionStart = codeEl.selectionEnd = s + 4;
    }
});

clearBtn.addEventListener("click", () => {
    if (confirm("محتوای ادیتور پاک شود؟")) {
        codeEl.value = "";
        codeEl.focus();
    }
});

clearOut.addEventListener("click", () => showOutput(""));

examples.addEventListener("change", () => {
    const key = examples.value;
    if (!key || !(key in SAMPLES)) return;
    codeEl.value = SAMPLES[key];
    examples.value = "";
});