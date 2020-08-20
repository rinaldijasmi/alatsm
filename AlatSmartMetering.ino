#include <Firebase.h>
#include <FirebaseArduino.h>
#include <FirebaseCloudMessaging.h>
#include <FirebaseError.h>
#include <FirebaseHttpClient.h>
#include <FirebaseObject.h>

#include <ESP8266WiFi.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#define FIREBASE_HOST "smartmetering-14c62.firebaseio.com"
#define FIREBASE_AUTH "CLU7XxgzJExeOPsssrHr2lvFQkQT6gnnuLiT2iJz"
#define WIFI_SSID "bungadii"
#define WIFI_PASSWORD "bacotbetsumpah"

const long utcOffsetInSeconds = 25200;

const int sensorIn = A0;
int mVperAmp = 185; // use 185 for 5A, 100 for 20A Module and 66 for 30A Module

double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;

// Setting nama hari dan tanggal
char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};


// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);

void setup() {
  pinMode(A0, INPUT);
  Serial.begin(115200);
  //connet to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  delay(10);
  Serial.println(F("Init...."));
}

void loop() {
  Voltage = getVPP();
  VRMS = (Voltage / 2.0) * 0.707; // sq root
  AmpsRMS = (VRMS * 1000) / mVperAmp;
  float Wattage = (3 * AmpsRMS); //Observed 18-20 Watt when no load was connected, so substracting offset value to get real consumption.

  //menghitung rataan harian
  double kWh = (Wattage * 24) / 1000;
  double harga = (kWh * 1467); //Untuk semua golongan karena Golongan R1,R2 dan R3 harga per kWh Rp 1476

  //menghitung total pemakaian selama sebulan
  double kWhB = (Wattage * 720) / 1000;

  //menghitung total pemakaian selama seminggu
  double kWhM = (Wattage * 168) / 1000;

  //Menghitung biaya energi listrik
  double hargaListrikMinggu =  (kWhM * 1467);

  //Menghitung biaya energi listrik
  double hargaListrikBulan =  (kWhB * 1467);

  //Menghitung ppj 1 Bulan
  double ppjBulan = (hargaListrikBulan * 0.06);
  
  //Menghitung ppj 1 Minggu
  double ppjMinggu = (hargaListrikMinggu * 0.06);

  //Menghitung ppn 1 Bulan
  double ppnBulan = (hargaListrikBulan * 0.10);
  
  //Menghitung ppn 1 Minggu
  double ppnMinggu = (hargaListrikMinggu * 0.10);

  //menghitung total biaya
  double hargaB1300 = (hargaListrikBulan + ppjBulan) ; //1 bulan RT-1/TR 901-1300VA
  double hargaM1300 = (hargaListrikMinggu + ppjMinggu); //1 minggu RT-1/TR 901-1300VA
  double hargaB2200 = (hargaListrikBulan + ppjBulan); //1 bulan RT-1/TR 1301-2200VA
  double hargaM2200 = (hargaListrikMinggu + ppjMinggu); //1 minggu RT-1/TR 1301-2200VA
  double hargaB5500 = (hargaListrikBulan + ppjBulan + ppnBulan); //1 bulan RT-2/TR 2201-5500VA
  double hargaM5500 = (hargaListrikMinggu + ppjMinggu + ppnMinggu); //1 minggu RT-2/TR 2201-5500VA
  double hargaB5501 = (hargaListrikBulan + ppjBulan + ppnBulan); //1 bulan RT-3/TR >5501VA
  double hargaM5501 = (hargaListrikMinggu + ppjMinggu + ppnMinggu); //1 minggu RT-3/TR >55001VA


  //Print Serial
  Serial.print(AmpsRMS);
  Serial.println(" Amps RMS ");
  Serial.print(Wattage);
  Serial.println(" Watt ");
  
  //Setting Jam
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());

  //Setting Tanggal
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int tanggal = ptm->tm_mday;
  int bulan = ptm->tm_mon+1;
  int tahun = ptm->tm_year+1900;
  String currentDate = String(tanggal) + "/" + String(bulan) + "/" + String(tahun);
  Serial.print("Current date: ");
  Serial.println(currentDate);

  //Input ke Firebase
  Firebase.setFloat("Watt", Wattage);
  Firebase.setFloat("Amp", AmpsRMS);
  Firebase.setFloat("kWh", kWh);
  Firebase.setFloat("harga", harga);
  Firebase.setFloat("kWhBulan", kWhB);
  Firebase.setFloat("kWhMinggu", kWhM);  
  Firebase.setFloat("hargalistrikminggu", hargaListrikMinggu); 
  Firebase.setFloat("hargalistrikbulan", hargaListrikBulan);
  Firebase.setFloat("ppjm", ppjMinggu);  
  Firebase.setFloat("ppjb", ppjBulan);  
  Firebase.setFloat("ppnm", ppnMinggu);  
  Firebase.setFloat("ppnb", ppnBulan);  
  Firebase.setFloat("hargaBulan1300", hargaB1300);
  Firebase.setFloat("hargaMinggu1300", hargaM1300);  
  Firebase.setFloat("hargaBulan2200", hargaB2200);
  Firebase.setFloat("hargaMinggu2200", hargaM2200);
  Firebase.setFloat("hargaBulan5500", hargaB5500);
  Firebase.setFloat("hargaMinggu5500", hargaM5500);
  Firebase.setFloat("hargaBulan5501", hargaB5501);
  Firebase.setFloat("hargaMinggu5501", hargaM5501);
 
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["watt"] = Wattage;
  root["arus"] = AmpsRMS;
  root["waktu"] = timeClient.getFormattedTime();
  root["hari"] = daysOfTheWeek[timeClient.getDay()];
  root["tanggal"] = currentDate;


  // append a new value to /logDHT
  String name = Firebase.push("/pemakaian", root);
  Serial.println("Masuk");


  if (Firebase.failed()) {
    Serial.print("Setting Failed");
    Serial.println(Firebase.error());
    return;
  }


}

float getVPP()
{
  float result;

  int readValue; //value read from the sensor
  int maxValue = 0; // store max value here
  int minValue = 1024; // store min value here

  uint32_t start_time = millis();

  while ((millis() - start_time) < 1000) //sample for 1 Sec
  {
    readValue = analogRead(sensorIn);
    // see if you have a new maxValue
    if (readValue > maxValue)
    {
      /*record the maximum sensor value*/
      maxValue = readValue;
    }
    if (readValue < minValue)
    {
      /*record the maximum sensor value*/
      minValue = readValue;
    }
    /* Serial.print(readValue);
      Serial.println(" readValue ");
      Serial.print(maxValue);
      Serial.println(" maxValue ");
      Serial.print(minValue);
      Serial.println(" minValue ");
      delay(1000); */
  }

  // Subtract min from max
  result = ((maxValue - minValue) * 5) / 1024.0;

  return result;
}
