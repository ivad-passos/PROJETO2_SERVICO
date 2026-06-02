from flask import Flask, request, jsonify
from database import init_db, inserir_leitura

app = Flask(__name__)

@app.route("/leitura", methods=["POST"])
def receber_leitura():
    dados = request.get_json()

    if not dados:
        return jsonify({"erro": "Nenhum dado recebido"}), 400

    campos_obrigatorios = ["nivel_tinta", "temperatura", "umidade", "luminosidade", "presenca"]
    for campo in campos_obrigatorios:
        if campo not in dados:
            return jsonify({"erro": f"Campo ausente: {campo}"}), 400

    inserir_leitura(
        nivel_tinta=dados["nivel_tinta"],
        temperatura=dados["temperatura"],
        umidade=dados["umidade"],
        luminosidade=dados["luminosidade"],
        presenca=dados["presenca"]
    )

    print(f"[LEITURA] nivel={dados['nivel_tinta']}% temp={dados['temperatura']}C "
          f"umd={dados['umidade']}% lux={dados['luminosidade']} prs={dados['presenca']}")

    return jsonify({"status": "ok"}), 200


if __name__ == "__main__":
    init_db()
    app.run(host="0.0.0.0", port=5000, debug=True)