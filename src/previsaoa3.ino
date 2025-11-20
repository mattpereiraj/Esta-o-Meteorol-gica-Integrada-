#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>

#define DHTPIN 23
#define DHTTYPE DHT11
#define SENSOR_CHUVA_PIN 34
#define UMIDADE_SOLO_PIN 35

const char* rede = "iPhone de Matheus";
const char* senha = "123456789";

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
WebServer server(80);
const long INTERVALO_TENDENCIA = 15000;
const long INTERVALO_TESTE = 10000;
unsigned long tempoUltimoTeste = 0;
int estadoTeste = 0;


float pressaoAnterior = 0.0;
String tendenciaPressao = "Calculando...";
unsigned long tempoUltimaTendencia = 0;

const int SOLO_SECO = 4095;
const int SOLO_MOLHADO = 1920;
const int NUM_AMOSTRAS = 10;
const int CHUVA_SECO = 4095;
const int CHUVA_MOLHADO = 1100;

const char* HTML_CSS = R"====(
<style>
html, body, header, h1, h2, h3, h4, div, nav, ul, li, img, p, section, article, a, button {
  padding: 0; margin: 0; list-style: none; box-sizing: border-box; outline: none;
  border: none; text-decoration: none; font-family: "Inter", sans-serif;
}
body { background: radial-gradient(circle, #4e80d0, #808aa5); }
.title { text-align: center; color: white; font-weight: bold; }
.container { max-width: 1300px; margin: 0 auto; display: flex; justify-content: space-between; }
.lado_direito { width: 70%; display: flex; gap: 10px; flex-wrap: wrap; align-content: center; flex-direction: column;
align-items: center; justify-content: space-around; padding: 40px; }
.cards { background-color: #a5c1ee; width: 90%; display: flex; gap: 30px 0; flex-wrap: wrap; align-content: center;
justify-content: space-around; padding: 20px; border-radius: 20px; transition: transform 0.3s ease; }
.card { width: 45%; height: 160px; text-align: center; border-radius: 15px; padding: px; display: flex; flex-direction: column;
justify-content: center; align-items: center; box-shadow: 0 4px 15px rgba(0,0,0,0.2); transition: transform 0.3s ease; }
.card:hover { transform: translateY(-5px); box-shadow: 0 8px 16px rgba(0,0,0,0.2); background: #2d5aa8; color: white; }
.precip { background-color: #a5c1ee; width: 90%; height: 300px; display: flex; justify-content: center; border-radius: 20px;
box-shadow: 0 4px 15px rgba(0,0,0,0.2); transition: transform 0.3s ease; }
.precip:hover { transform: translateY(-5px); box-shadow: 0 8px 16px rgba(0,0,0,0.2); }
.var { width: 100%; height: calc(100% - 30px); display: flex; justify-content: center; align-content: center;
align-items: center; font-size: 30px; font-weight: bold; }
.titulo { font-size: 18px; font-weight: bold; height: 30px; }
.lado_esquerdo { display: flex; width: 40vw; height: 90vh; align-items: center; justify-content: center; }
.barra1 { position: relative; display: flex; flex-direction: column; align-items: center; justify-content: flex-end;
background: linear-gradient(180deg, #7289da, #99aaff); width: 85%; height: 90%; overflow: hidden;
border-radius: 16px; box-shadow: inset 0 0 10px rgba(0,0,0,0.3), 0 4px 10px rgba(0,0,0,0.4); }
.titulob { position: absolute; top: 10px; left: 50%; transform: translateX(-50%);
background: rgba(255,255,255,0.15); color: #e0e7ff; padding: 6px 18px; border-radius: 10px; font-size: 1rem;
font-weight: bold; backdrop-filter: blur(8px); box-shadow: 0 2px 8px rgba(0,0,0,0.2); }
.nivel { width: 60%; height: 0%; background: linear-gradient(180deg, #6ee4b3, #3dcf2a); border-radius: 8px 8px 0 0;
transition: height 0.6s ease; box-shadow: inset 0 0 8px rgba(255,255,255,0.3); }
</style>
)====";

const char* HTML_BODY = R"====(
<!DOCTYPE html>
<html lang="pt">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Dashboard A3</title>
)====";

const char* HTML_END = R"====(
  </head>
  <body>
     <header>
      <h1 class="title">Dashboard A3</h1>
    </header>
    <main>
      <div class="container">
        <aside class="lado_esquerdo">
          <article class="barra1">
            <h3 class="titulob">Umidade Do Solo</h3>
            <div class="nivel"></div>
          </article>
        </aside>
        <div class="lado_direito">
          <article class="cards">
            
            <section class="card">
              <div class="titulo"><h3>Umidade Atmosferica</h3></div>
              <div class="var umidade-valor"></div>
            </section>
            
            <section class="card">
              <div class="titulo"><h3>Pressao Atmosferica</h3></div>
              <div class="var pressao-valor"></div>
            </section>
            
            <section class="card">
              <div class="titulo"><h3>Temperatura</h3></div>
              <div class="var temperatura-valor"></div>
            </section>
            
            <section class="card">
              <div class="titulo"><h3>Previsão (Próx. Horas)</h3></div>
              <div class="var previsao-valor" style="padding: 0 10px;"></div>
            </section>

          </article>
          <article class="precip">
            <section class="precipitacao-valor"></section>
          </article>
        </div>
      </div>
    </main>
    <script>
      function atualizarUmidade(v){document.querySelector(".umidade-valor").innerHTML=`<p>${v}%</p>`;}
      
      function atualizarPressao(v){
        const valor_formatado = v.toString().replace(".", ",");
        document.querySelector(".pressao-valor").innerHTML=`<p>${valor_formatado} atm</p>`;
      }
      
      function atualizarTemperatura(v){document.querySelector(".temperatura-valor").innerHTML=`<p>${v}°C</p>`;}
      
      function atualizarNivel(v){const b=document.querySelector(".nivel");v=Math.min(Math.max(v,0),100);b.style.height=v+"%";}
      
      function atualizarPrecipitacao(v){
        const p=document.querySelector(".precipitacao-valor");let emoji="",msg="";
        if(v<=10){emoji="☀️";msg="Risco Muito Baixo";}
        else if(v<=30){emoji="☁️";msg="Risco Baixo";}
        else if(v<=50){emoji="🌧️";msg="Risco Moderado";}
        else if(v<=85){emoji="☔";msg="Risco Alto";}
        else{emoji="⛈️";msg="Risco Muito Alto";}
        p.innerHTML=`<div style="font-size:28px;font-weight:bold;text-align:center;">
          <h3>Precipitação</h3>
          <div style="font-size:80px;margin:20px auto;">${emoji}</div>
          <div><p style="font-size:26px;">${v}%</p><p style="font-size:20px;">${msg}</p></div></div>`;
      }
      
      function atualizarPrevisao(tendencia, umidade) {
        const p = document.querySelector(".previsao-valor");
        let emoji = "";
        let msg = "";

        if (tendencia === "Caindo") {
          if (umidade > 75) {
            emoji = "⛈️"; msg = "Chuva Forte / Tempestade";
          } else {
            emoji = "🌧️"; msg = "Chuva a Caminho";
          }
        } else if (tendencia === "Subindo") {
          emoji = "☀️"; msg = "Tempo Bom / Limpando";
        } else { // "Estável" ou "Calculando..."
          if (tendencia === "Calculando...") {
             emoji = "🤔"; msg = "Calculando...";
          } else if (umidade > 80) {
            emoji = "☁️"; msg = "Nublado / Possível Chuva";
          } else if (umidade < 40) {
            emoji = "☀️"; msg = "Tempo Estável e Seco";
          } else {
            emoji = "🌤️"; msg = "Tempo Estável";
          }
        }
        p.innerHTML = `<div style="font-size:48px; margin-bottom:10px;">${emoji}</div><p style="font-size:22px;">${msg}</p>`;
      }

      function buscar(){
        fetch("/data").then(r=>r.json()).then(d=>{
          atualizarUmidade(d.umidade);
          atualizarPressao(d.pressao);
          atualizarTemperatura(d.temperatura);
          atualizarNivel(d.umidade_solo);
          atualizarPrecipitacao(d.precipitacao);
          atualizarPrevisao(d.tendencia, d.umidade); 
        }).catch(e=>console.error("Erro:",e));
      }
      buscar(); setInterval(buscar,5000);
    </script>
  </body>
</html>
)====";

void handleRoot() {
  String html = "";
  html += HTML_BODY;
  html += HTML_CSS;
  html += HTML_END;
  server.send(200, "text/html", html);
}

float lerUmidadeSoloCalibrada() {
  long soma = 0;
  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    soma += analogRead(UMIDADE_SOLO_PIN);
    delay(10);
  }
  int media = soma / NUM_AMOSTRAS;
  float porcentagem = map(media, SOLO_SECO, SOLO_MOLHADO, 0, 100);
  porcentagem = constrain(porcentagem, 0, 100);
  return porcentagem;
}

float lerNivelChuva() {
  int valorChuva = analogRead(SENSOR_CHUVA_PIN);
  float porcentagem = map(valorChuva, CHUVA_SECO, CHUVA_MOLHADO, 0, 100);
  porcentagem = constrain(porcentagem, 0, 100);
  return porcentagem;
}

void handleSensorData() {
  float temperatura_dht = dht.readTemperature();
  float umidade = dht.readHumidity();
  
  if (isnan(umidade) || isnan(temperatura_dht)) {
    Serial.println("❌ Erro ao ler do sensor DHT!");
    return;
  }
  float calor = dht.computeHeatIndex(temperatura_dht, umidade, false);

  float temperatura_bmp = bmp.readTemperature();
  float pressao_hPa = bmp.readPressure() / 100.0;
  float pressao_atm = pressao_hPa / 1013.25;
  float altitude = bmp.readAltitude(1013.25);

  float umidade_solo = lerUmidadeSoloCalibrada();
  float precipitacao = lerNivelChuva();

  Serial.println("=== LEITURAS ===");
  Serial.printf("Temp DHT: %.1f °C | Temp BMP: %.1f °C\n", temperatura_dht, temperatura_bmp);
  Serial.printf("Umidade: %.1f %% | Índice de Calor: %.1f °C\n", umidade, calor);
  Serial.printf("Pressão: %.1f hPa (%.3f atm)\n", pressao_hPa, pressao_atm);
  Serial.printf("Umidade Solo: %.1f %%\n", umidade_solo);
  Serial.printf("Nível Chuva: %.1f %%\n", precipitacao);
  Serial.println("=================\n");

  String json = "{";
  json += "\"temperatura\": " + String(temperatura_bmp, 1);
  json += ", \"umidade\": " + String(umidade, 1);
  json += ", \"calor\": " + String(calor, 1);
  json += ", \"pressao\": " + String(pressao_atm, 3);
  json += ", \"umidade_solo\": " + String(umidade_solo, 1);
  json += ", \"precipitacao\": " + String(precipitacao, 1);
  json += ", \"altitude\": " + String(altitude, 1);
  json += ", \"tendencia\": \"" + tendenciaPressao + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("❌ Erro ao iniciar BMP180! Verifique conexões SDA/SCL (pinos 21 e 22).");
    while (1);
  }

  analogReadResolution(12);
  WiFi.begin(rede, senha);
  Serial.print("Conectando ao WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ Conectado ao WiFi!");
  Serial.print("🌐 IP do ESP32: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleSensorData);
  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Rota não encontrada");
  });

  server.begin();
  Serial.println("Servidor iniciado!");
}

void loop() {
  server.handleClient();

  unsigned long agora = millis();

  if (agora - tempoUltimaTendencia > INTERVALO_TENDENCIA) { 
    float pressaoAtual = bmp.readPressure() / 100.0;
    if (pressaoAtual > 0 && !isnan(pressaoAtual)) {
      if (pressaoAnterior == 0.0) {
        pressaoAnterior = pressaoAtual;
        tendenciaPressao = "Estável";
      } else {
        float deltaPressao = pressaoAtual - pressaoAnterior;
        if (deltaPressao > 2.0) {
          tendenciaPressao = "Subindo";
        } else if (deltaPressao < -2.0) {
          tendenciaPressao = "Caindo";
        } else {
          tendenciaPressao = "Estável";
        }
        pressaoAnterior = pressaoAtual;
      }
      Serial.printf("[PREVISÃO] Nova tendência: %s (Pressão: %.1f hPa)\n", tendenciaPressao.c_str(), pressaoAtual);
      tempoUltimaTendencia = agora;
    }
  }
  // unsigned long agora = millis();
  // if (agora - tempoUltimoTeste > INTERVALO_TESTE) {
  //   if (estadoTeste == 0) {
  //     tendenciaPressao = "Estável";
  //     Serial.println("[TESTE] Mudando previsão para: Estável");
  //     estadoTeste = 1;
  //   } else if (estadoTeste == 1) {
  //     tendenciaPressao = "Caindo";
  //     Serial.println("[TESTE] Mudando previsão para: Caindo");
  //     estadoTeste = 2;    
  //   } else {
  //     tendenciaPressao = "Subindo";
  //     Serial.println("[TESTE] Mudando previsão para: Subindo");
  //     estadoTeste = 0;
  //   }
  //   tempoUltimoTeste = agora;
  // }
}