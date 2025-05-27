let token = "";

function login() {
    const email = document.getElementById("email").value;
    const password = document.getElementById("password").value;

    fetch("http://localhost:3001/login", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ email, password })
    })
    .then(res => res.json())
    .then(data => {
        if (data.token) {
            token = data.token;
            alert("Успешный вход!");
        } else {
            alert("Ошибка входа");
        }
    });
}

function addLed() {
    const ledCode = document.getElementById("ledCode").value;

    fetch("http://localhost:3001/add-led-strip", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ userId: 1, code: ledCode })
    })
    .then(res => res.json())
    .then(data => alert(data.success ? "Лента добавлена!" : "Ошибка!"));
}

function setMode(mode) {
    const ledCode = document.getElementById("ledCode").value;

    fetch("http://localhost:3001/update-led-strip", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ userId: 1, code: ledCode, mode, color: "#FFFFFF" })
    })
    .then(res => res.json())
    .then(data => alert(data.success ? "Режим изменен!" : "Ошибка!"));
}

function setColor(color) {
    const ledCode = document.getElementById("ledCode").value;

    fetch("http://localhost:3001/update-led-strip", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ userId: 1, code: ledCode, mode: "manual", color })
    })
    .then(res => res.json())
    .then(data => alert(data.success ? "Цвет изменен!" : "Ошибка!"));
}
