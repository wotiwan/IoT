const express = require("express");
const sqlite3 = require("sqlite3").verbose();
const bcrypt = require("bcryptjs");
const session = require("express-session");
const bodyParser = require("body-parser");
const path = require('path');

const app = express();
app.set("view engine", "ejs");
app.use(express.static("public"));
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(session({ secret: "secret-key", resave: false, saveUninitialized: true }));

const db = new sqlite3.Database("./database.db");

const expressLayouts = require("express-ejs-layouts");
app.use(expressLayouts);
app.use(express.static(path.join(__dirname, 'public')));
app.set("layout", "layout"); 


// **Создание таблиц**
db.serialize(() => {
  db.run("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, username TEXT, email TEXT UNIQUE, password TEXT)");

  db.run(`CREATE TABLE IF NOT EXISTS led_strips (
    id INTEGER PRIMARY KEY,
    code TEXT UNIQUE,
    user_id INTEGER,
    admin_user_id INTEGER,
    mode TEXT,
    color TEXT,
    is_on INTEGER DEFAULT 1,
    saturation INTEGER DEFAULT 255,
    brightness INTEGER DEFAULT 85,
    speed INTEGER DEFAULT 10
  )`);
});



// **Главная страница (авторизация)**
app.use((req, res, next) => {
  res.locals.user = req.session.userId ? { id: req.session.userId, login: req.session.username, email: req.session.email } : null;
  next();
});
app.use((req, res, next) => {
  res.locals.username = req.session.user ? req.session.user.login : null;
  next();
});
app.get("/", (req, res) => {
  res.render("login", { title: "Вход", username: req.session.username });
});

app.use((req, res, next) => {
  res.locals.title = "Сайт управления лентой";
  next();
});


// **Страница регистрации**
app.get("/register", (req, res) => {
  res.render("register", { title: "Регистрация" });
});

// **Страница аккаунта**
app.get("/account", (req, res) => {
  if (!req.session.userId) return res.redirect("/");
  
  db.all("SELECT * FROM led_strips WHERE user_id = ?", [req.session.userId], (err, strips) => {
      if (err) return res.status(500).send("Ошибка при получении лент");

      res.render("account", { 
          title: "Мой Аккаунт", 
          username: req.session.username, 
          email: req.session.email, 
          strips, 
          user: { id: req.session.userId }
      });
  });
});

// **Страница управления конкретной лентой**
app.get("/led-control/:code", (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  const { code } = req.params;

  db.get("SELECT * FROM led_strips WHERE user_id = ? AND code = ?", [req.session.userId, code], (err, strip) => {
    if (!strip) return res.redirect("/account");

    res.render("led-control", { title: "Управление лентой", strip });
  });
});

// **Регистрация пользователя**
app.post("/register", async (req, res) => {
  const { email, password } = req.body;

  // Проверяем, существует ли пользователь с таким email
  db.get("SELECT * FROM users WHERE email = ?", [email], async (err, existingUser) => {
    if (err) {
      return res.send("Ошибка базы данных");
    }
    if (existingUser) {
      return res.send("Пользователь с такой почтой уже зарегистрирован");
    }

    // Хешируем пароль
    const hashedPassword = await bcrypt.hash(password, 10);

    db.run("INSERT INTO users (email, password) VALUES (?, ?)", [email, hashedPassword], function (err) {
      if (err) {
        return res.send("Ошибка при регистрации");
      }
      res.redirect("/login");
    });
  });
});

// **Авторизация пользователя**
app.get("/login", (req, res) => {
  res.render("login");
});
app.post("/login", (req, res) => {
  const { email, password } = req.body;

  db.get("SELECT * FROM users WHERE email = ?", [email], (err, user) => {
    if (!user || !bcrypt.compareSync(password, user.password)) {
      return res.send("Ошибка входа");
    }
    req.session.userId = user.id;
    req.session.username = user.username;
    req.session.email = user.email;
    res.redirect("/account");
  });
});

// **Выход**
app.get("/logout", (req, res) => {
  req.session.destroy(() => {
    res.redirect("/");
  });
});

// **Скачать ПО**
app.get('/download', (req, res) => {
  res.render('download', {
    title: 'Скачать ПО',
    username: req.user?.username || null
  });
});

// **Страница админа**
app.get('/admin-panel', (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  const userId = req.session.userId; // Используем session.userId

  db.all(`SELECT * FROM led_strips WHERE admin_user_id = ?`, [userId], (err, adminStrips) => {
    if (err) return res.status(500).send("Ошибка получения лент");

    const stripCodes = adminStrips.map(s => s.code);
    if (stripCodes.length === 0) return res.render('admin', { strips: [] });

    db.all(
      `SELECT * FROM led_strips WHERE code IN (${stripCodes.map(() => '?').join(',')})`,
      stripCodes,
      (err, allUsersSettings) => {
        if (err) return res.status(500).send("Ошибка получения настроек пользователей");
        res.render('admin', { strips: adminStrips, usersSettings: allUsersSettings });
      }
    );
  });
});

// **Сброс настроек**
app.post('/admin/reset-settings', (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  const { code, user_id } = req.body; // Извлекаем данные из формы

  db.get(`SELECT * FROM led_strips WHERE code = ? AND admin_user_id = ?`, [code, req.session.userId], (err, adminStrip) => {
    if (!adminStrip) return res.status(403).send("Нет прав");

    db.run(`DELETE FROM led_strips WHERE code = ? AND user_id = ?`, [code, user_id], (err) => {
      if (err) return res.status(500).send("Ошибка при сбросе настроек");
      res.redirect('/admin-panel');
    });
  });
});



// **Привязка ленты**
app.post("/add-led-strip", (req, res) => {
  if (!req.session.userId) return res.redirect("/");
  const { code } = req.body;
  
  db.run("INSERT INTO led_strips (user_id, code, mode, color) VALUES (?, ?, 'manual', '#FFFFFF')", 
         [req.session.userId, code], function (err) {
    if (err) return res.send("Ошибка при привязке");

    db.get("SELECT * FROM led_strips WHERE code = ?", [code], (err, strip) => {
      if (!strip.admin_user_id) {
        db.run("UPDATE led_strips SET admin_user_id = ? WHERE code = ?", [req.session.userId, code]);
      }
    });

    res.redirect("/account");
  });
});


// **Изменение режима/цвета ленты**
app.post("/update-led-strip", (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  const { code, is_on, mode, color, saturation, brightness, speed } = req.body;

  db.run(
    `UPDATE led_strips 
     SET is_on = ?, mode = ?, color = ?, saturation = ?, brightness = ?, speed = ? 
     WHERE user_id = ? AND code = ?`,
    [is_on, mode, color || "#FFFFFF", saturation, brightness, speed, req.session.userId, code],
    (err) => {
      if (err) return res.send("Ошибка при обновлении ленты");

      res.redirect(`/account`);
    }
  );
});

// **Обновляем только указанный параметр**
app.post("/update-led-parameter", (req, res) => {
  if (!req.session.userId) return res.status(401).send("Неавторизован");

  const { code, parameter, value } = req.body;

  const allowedParams = ["brightness", "speed"];
  if (!allowedParams.includes(parameter)) {
    return res.status(400).send("Неверный параметр");
  }

  
  const sql = `UPDATE led_strips SET ${parameter} = ? WHERE user_id = ? AND code = ?`;
  db.run(sql, [value, req.session.userId, code], function(err) {
    if (err) return res.status(500).send("Ошибка обновления");

    res.send({ success: true, parameter, value });
  });
});



// **Запуск сервера**
app.listen(3001, () => console.log("Сервер запущен на порту 3001"));
