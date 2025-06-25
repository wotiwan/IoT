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
    speed INTEGER DEFAULT 10,
    width INTEGER DEFAULT 60,
    height INTEGER DEFAULT 60,
    direction TEXT DEFAULT 'right'
    )
    `);
});


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

app.get("/register", (req, res) => {
  res.render("register", { title: "Регистрация" });
});


app.get("/account", (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  db.get("SELECT * FROM users WHERE id = ?", [req.session.userId], (err, userData) => {
    if (err) return res.status(500).send("Ошибка при получении данных пользователя");

    db.all("SELECT * FROM led_strips WHERE user_id = ?", [req.session.userId], (err, strips) => {
      if (err) return res.status(500).send("Ошибка при получении лент");

      res.render("account", {
        title: "Мой Аккаунт",
        username: userData.username,
        email: userData.email,
        strips,
        user: userData
      });
    });
  });
});



app.get("/led-control/:code", (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  const { code } = req.params;

  db.get("SELECT * FROM led_strips WHERE user_id = ? AND code = ?", [req.session.userId, code], (err, strip) => {
    if (!strip) return res.redirect("/account");

    res.render("led-control", { title: "Управление лентой", strip });
  });
});

app.post("/register", async (req, res) => {
  const { username, email, password} = req.body;

  db.get("SELECT * FROM users WHERE email = ?", [email], async (err, existingUser) => {
    if (err) return res.send("Ошибка базы данных");
    if (existingUser) return res.send("Пользователь с такой почтой уже зарегистрирован");

    const hashedPassword = await bcrypt.hash(password, 10);

    db.run("INSERT INTO users (username, email, password) VALUES (?, ?, ?)",
      [username, email, hashedPassword],
      function (err) {
        if (err) {
          console.error("Ошибка при регистрации:", err.message);
          return res.send("Ошибка при регистрации: " + err.message);
        }

        req.session.userId = this.lastID;
        req.session.username = username;
        req.session.email = email;
        res.redirect("/account");
      });
  });
});


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


app.get("/logout", (req, res) => {
  req.session.destroy(() => {
    res.redirect("/");
  });
});


app.get('/download', (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  db.get("SELECT * FROM led_strips WHERE user_id = ?", [req.session.userId], (err, strip) => {
    if (err || !strip) return res.render('download', { title: 'Скачать ПО', strip: { code: '' } });

    res.render('download', {
      title: 'Скачать ПО',
      strip
    });
  });
});


app.get('/admin-panel', (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  const userId = req.session.userId; 

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

app.post('/admin/reset-settings', (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  const { code, user_id } = req.body; 

  db.get(`SELECT * FROM led_strips WHERE code = ? AND admin_user_id = ?`, [code, req.session.userId], (err, adminStrip) => {
    if (!adminStrip) return res.status(403).send("Нет прав");

    db.run(`DELETE FROM led_strips WHERE code = ? AND user_id = ?`, [code, user_id], (err) => {
      if (err) return res.status(500).send("Ошибка при сбросе настроек");
      res.redirect('/admin-panel');
    });
  });
});



app.post("/add-led-strip", (req, res) => {
  if (!req.session.userId) return res.redirect("/");
  const { code } = req.body;
  
  db.run("INSERT INTO led_strips (user_id, code, mode, color) VALUES (?, ?, 'manual', '#FFFFFF')", 
         [req.session.userId, code], function (err) {
    if (err) return res.send("Ошибка при привязке");

    req.session.stripCode = code;                                                                   

    db.get("SELECT * FROM led_strips WHERE code = ?", [code], (err, strip) => {
      if (!strip.admin_user_id) {
        db.run("UPDATE led_strips SET admin_user_id = ? WHERE code = ?", [req.session.userId, code]);
      }
    });

    res.redirect("/account");
  });
});

app.post("/update-led-strip", (req, res) => {
  if (!req.session.userId) return res.redirect("/");

  const { code, is_on, mode, color, saturation, brightness, speed, direction } = req.body;

  db.run(
    `UPDATE led_strips 
     SET is_on = ?, mode = ?, color = ?, saturation = ?, brightness = ?, speed = ?, direction = ?
     WHERE user_id = ? AND code = ?`,
    [is_on, mode, color || "#FFFFFF", saturation, brightness, speed, direction || 'right', req.session.userId, code],
    (err) => {
      if (err) return res.send("Ошибка при обновлении ленты");
      res.redirect(`/account`);
    }
  );
});

app.post("/update-led-parameter", (req, res) => {
  if (!req.session.userId) return res.status(401).send("Неавторизован");

  const { code, parameter, value } = req.body;

  const allowedParams = ["brightness", "speed","height","width"];
  if (!allowedParams.includes(parameter)) {
    return res.status(400).send("Неверный параметр");
  }

  
  const sql = `UPDATE led_strips SET ${parameter} = ? WHERE user_id = ? AND code = ?`;
  db.run(sql, [value, req.session.userId, code], function(err) {
    if (err) return res.status(500).send("Ошибка обновления");

    res.send({ success: true, parameter, value });
  });
});


app.listen(3001, () => console.log("Сервер запущен на порту 3001"));
