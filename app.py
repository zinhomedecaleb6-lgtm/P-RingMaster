from functools import wraps
import sqlite3
from flask import Flask, redirect, render_template, request, session, url_for

app = Flask(__name__)
app.secret_key = (
    "votre_cle_secrete_tres_securisee"  # Indispensable pour gérer les sessions
)

DB_NAME = "p_ringmaster.db"


def init_db():
  conn = sqlite3.connect(DB_NAME)
  cursor = conn.cursor()

  # Table pour les écoles
  cursor.execute("""
        CREATE TABLE IF NOT EXISTS schools (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            code TEXT NOT NULL
        )
    """)

  # Table pour les horaires de sonnerie
  cursor.execute("""
        CREATE TABLE IF NOT EXISTS schedules (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            school_id INTEGER,
            time TEXT NOT NULL,
            label TEXT NOT NULL,
            FOREIGN KEY (school_id) REFERENCES schools (id)
        )
    """)

  conn.commit()
  conn.close()


# Initialisation de la base de données au lancement
init_db()


# Décorateur pour protéger les pages (exige la connexion)
def login_required(f):
  @wraps(f)
  def decorated_function(*args, **kwargs):
    if "school_id" not in session:
      return redirect(url_for("login"))
    return f(*args, **kwargs)

  return decorated_function


@app.route("/")
@login_required
def index():
  return render_template("index.html")


@app.route("/login", methods=["GET", "POST"])
def login():
  if request.method == "POST":
    school_name = request.form.get("school_name")
    school_code = request.form.get("school_code")

    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()

    # Vérifier si l'école existe déjà ou l'enregistrer
    cursor.execute(
        "SELECT id FROM schools WHERE name = ? AND code = ?",
        (school_name, school_code),
    )
    school = cursor.fetchone()

    if not school:
      # Inscription automatique si l'école n'existe pas
      cursor.execute(
          "INSERT INTO schools (name, code) VALUES (?, ?)",
          (school_name, school_code),
      )
      conn.commit()
      cursor.execute(
          "SELECT id FROM schools WHERE name = ? AND code = ?",
          (school_name, school_code),
      )
      school = cursor.fetchone()

    conn.close()

    # Enregistrement de la session
    session["school_id"] = school[0]
    session["school_name"] = school_name
    return redirect(url_for("index"))

  return render_template("login.html")


@app.route("/logout")
def logout():
  session.clear()  # Nettoie toute la session
  return redirect(url_for("login"))


if __name__ == "__main__":
  app.run(debug=True)