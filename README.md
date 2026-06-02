# Robo Lab - Sistema IoT de Pintura de Blocos

Sistema IoT para monitoramento do processo de pintura de blocos de madeira, usando dois ESP32 comunicando via ESP-NOW, backend Python/Flask, banco de dados MySQL e dashboard Grafana.

## Estrutura do Projeto

```
PROJETO2_SERVICO/
├── esp32/
│   ├── chao_de_fabrica/    # Firmware do ESP32 que lê os sensores
│   └── monitoramento/      # Firmware do ESP32-S3 que exibe na matriz e envia para API
├── backend/
│   ├── app.py              # API Flask
│   ├── database.py         # Conexão e criação do banco MySQL
│   └── requirements.txt    # Dependências Python
└── grafana/
    ├── dashboard.json      # Dashboard exportado
    └── datasource.json     # Configuração da fonte de dados MySQL
```

## Como Montar o Projeto

### ESP32 Chão de Fábrica
| Componente | Pino ESP32 |
|---|---|
| DHT22 (dados) | GPIO 15 |
| LDR | GPIO 34 |
| HC-SR04 TRIG | GPIO 5 |
| HC-SR04 ECHO | GPIO 27 |
| LED Verde | GPIO 26 |
| LED Vermelho | GPIO 25 |

### ESP32-S3 Monitoramento
| Componente | Pino ESP32-S3 |
|---|---|
| MAX7219 DIN | GPIO 4 |
| MAX7219 CLK | GPIO 6 |
| MAX7219 CS | GPIO 5 |
| LED Verde | GPIO 26 |
| LED Vermelho | GPIO 25 |

## Como Rodar o Backend

1. Instalar dependências:
```bash
pip install -r backend/requirements.txt
```

2. Editar `backend/database.py` com sua senha do MySQL

3. Iniciar o servidor:
```bash
python backend/app.py
```

A API ficará disponível em `http://<SEU_IP>:5000/leitura`

## Como Configurar o Grafana

1. Acesse `http://localhost:3000` (login: admin / admin)

2. Adicionar fonte de dados MySQL:
   - Connections → Data Sources → Add → MySQL
   - Host: `localhost:3306`
   - Database: `robo_lab`
   - User: `root` / senha do MySQL

3. Importar dashboard:
   - Dashboards → New → Import
   - Fazer upload do arquivo `grafana/dashboard.json`

## Como Consultar Dados Salvos

Via MySQL:
```sql
SELECT * FROM robo_lab.leituras ORDER BY timestamp DESC LIMIT 20;
```

Via API (último registro):
```bash
curl http://localhost:5000/leitura
```
