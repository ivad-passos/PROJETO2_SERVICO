import mysql.connector
from mysql.connector import Error

# =====================
# CONFIGURACAO DO BANCO
# =====================
DB_CONFIG = {
    "host": "localhost",
    "user": "root",
    "password": "123456",
    "database": "robo_lab"
}

# =====================
# INICIALIZAR BANCO
# =====================
def init_db():
    try:
        conn = mysql.connector.connect(
            host=DB_CONFIG["host"],
            user=DB_CONFIG["user"],
            password=DB_CONFIG["password"]
        )
        cursor = conn.cursor()

        cursor.execute("CREATE DATABASE IF NOT EXISTS robo_lab")
        cursor.execute("USE robo_lab")

        cursor.execute("""
            CREATE TABLE IF NOT EXISTS leituras (
                id INT AUTO_INCREMENT PRIMARY KEY,
                nivel_tinta INT NOT NULL,
                temperatura FLOAT NOT NULL,
                umidade FLOAT NOT NULL,
                luminosidade INT NOT NULL,
                presenca TINYINT(1) NOT NULL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        """)

        conn.commit()
        print("[DB] Banco de dados inicializado com sucesso")

    except Error as e:
        print(f"[DB] Erro ao inicializar banco: {e}")

    finally:
        if conn.is_connected():
            cursor.close()
            conn.close()

# =====================
# INSERIR LEITURA
# =====================
def inserir_leitura(nivel_tinta, temperatura, umidade, luminosidade, presenca):
    try:
        conn = mysql.connector.connect(**DB_CONFIG)
        cursor = conn.cursor()

        cursor.execute("""
            INSERT INTO leituras (nivel_tinta, temperatura, umidade, luminosidade, presenca)
            VALUES (%s, %s, %s, %s, %s)
        """, (nivel_tinta, temperatura, umidade, luminosidade, presenca))

        conn.commit()
        print(f"[DB] Leitura inserida — ID {cursor.lastrowid}")

    except Error as e:
        print(f"[DB] Erro ao inserir leitura: {e}")

    finally:
        if conn.is_connected():
            cursor.close()
            conn.close()