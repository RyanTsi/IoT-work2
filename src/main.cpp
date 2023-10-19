#include <Arduino.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_SSD1306.h>
#include <HttpClient.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 // 屏幕宽度
#define SCREEN_HEIGHT 64 // 屏幕高度

#define OLED_MOSI  12   // D1
#define OLED_CLK   14   // D0
#define OLED_DC    0
#define OLED_CS    4
#define OLED_RESET 2

// wifi 信息
const char *ssid     = "YOUR SSID";
const char *password = "YOUR PASSWORD";

WiFiUDP ntpUDP;
WiFiClient wifiClient;
HttpClient client = HttpClient(wifiClient, "api.seniverse.com", 80);
NTPClient timeClient(ntpUDP);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

uint32_t delayMS = 1000; // 循环时间

// 天气信息类
class WeatherInfo {
public:
    String date;
    int highTemperature;
    int lowTemperature;
    int humidity;

    WeatherInfo() {
        date = "";
        highTemperature = 0;
        lowTemperature = 0;
        humidity = 0;
    }
};
// 储存近三天的天气的数组
std::vector<WeatherInfo> WeatherInfos; 
// json 解析得到近三天的天气
void parseWeatherJSON() {
    WeatherInfos.clear();
    // HTTP 访问 API，下面的 key=YOURKEY 填写自己的密钥
    client.get("/v3/weather/daily.json?key=YOURKEY&location=hangzhou&language=zh-Hans&unit=c&start=0&days=5");
    // 读取响应
    String json = "";
    while (client.available()) {
        char c = client.read();
        json += c;
    }
    Serial.println(json);
    // 根据你的 JSON 数据大小分配足够的内存
    StaticJsonDocument<2048> doc;
    deserializeJson(doc, json);
    // 获取 "daily" 数组
    JsonArray daily = doc["results"][0]["daily"];
    // 创建 WeatherInfo 对象并设置相应属性
    for (int i = 0; i < 3; i++) { // 获取近三天的数据
        WeatherInfo weather;
        weather.date = daily[i]["date"].as<String>();
        weather.highTemperature = daily[i]["high"].as<int>();
        weather.lowTemperature = daily[i]["low"].as<int>();
        weather.humidity = daily[i]["humidity"].as<int>();
        WeatherInfos.push_back(weather);
    }
}
// 显示近三天的天气到屏幕上
void weather_disPlay() {
    parseWeatherJSON();
    delay(delayMS);
    display.clearDisplay();
    display.println("  date     low   high\n");
    for(int i = 0; i < 3; i ++) {
        display.print(WeatherInfos[i].date);
        display.print("  ");
        if(WeatherInfos[i].lowTemperature < 10) display.print("0");
        display.print(WeatherInfos[i].lowTemperature);
        display.print("    ");
        display.println(WeatherInfos[i].highTemperature);
        // display.print(" ");
        // display.println(WeatherInfos[i].humidity);
    }
    display.display();
}
void setup(){

    Serial.begin(115200);

    if (!display.begin(SSD1306_SWITCHCAPVCC)) {
        Serial.println("failed");
        for (;;);
    } else {
        Serial.print("ssd1306 is ok");
    }

    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.clearDisplay();
    
    WiFi.begin(ssid, password);
    
    while ( WiFi.status() != WL_CONNECTED ) {
        delay ( delayMS );
        Serial.print ( "." );
    }

    timeClient.begin();
    timeClient.setTimeOffset(8 * 3600);

    delay(delayMS);

    weather_disPlay();
}

void loop() {
    timeClient.update();
    // 每天零点更新天气
    if(timeClient.getHours() * 3600 + timeClient.getMinutes() * 60 + timeClient.getSeconds() <= 2) {
        weather_disPlay();
    }
    delay(delayMS);
}