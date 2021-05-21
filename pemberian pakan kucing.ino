#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <FirebaseArduino.h>
#include <Servo.h>
#include "HX711.h"
#include <ESP8266HTTPClient.h>


String url; // url ifft
Servo myservo;

//Define Wifi,Firebase dan NTP Client
#define WIFI_SSID "Family"
#define WIFI_PASSWORD "271294qwas"
#define firebase_host "pakanku-70004.firebaseio.com"
#define firebase_key  "Kqpjyo5pgA6aNeZ1qsmMrJUieDQkyni0fGunKYba"
const long selisih_waktu = 25200; // selisih waktu zona dengan UTC
int jam_ntp,menit_ntp,detik_ntp;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.id.pool.ntp.org", selisih_waktu); 

// Define variabel data_firebase
String porsi_sehari,porsi_sekali,jadwal1,jadwal2,jadwal3,jadwal4;
float seporsi;
int jam1,menit1,jam2,menit2,jam3,menit3,jam4,menit4,detik;

//Define pin ultrasonic
#define pinTrigger  D3
#define pinEcho     D4
long durasi;
float kecil_pakan=200;
float jarak,jarak_pakan,volume,sisa_pakan;

//Define pin HX711
#define DOUT  D2
#define CLK  D1
HX711 scale(DOUT, CLK);
float faktor_kalibrasi =-720.10;
float berat_pakan=0,berat=0,berat_kecil=0;
float takaran=0,terkecil=0,stat;
 
void setup(){
Serial.begin(9600);
myservo.attach(D5);

// Koneksi Wifi
WiFi.begin(WIFI_SSID , WIFI_PASSWORD);
while ( WiFi.status() != WL_CONNECTED ) {
delay ( 500 );
Serial.print ( "." );
}
  Serial.println();
  Serial.print("Connected");
  Serial.println(WiFi.localIP());
// Koneksi Firebase
Firebase.begin(firebase_host,firebase_key);
// Koneksi ke NTP client
timeClient.begin();
 
// Ultrasonic
pinMode(pinTrigger, OUTPUT);
pinMode(pinEcho, INPUT);

// HX711
scale.set_scale();
scale.tare();  
}

void ultrasonic(){
// Ultrasonic1
digitalWrite(pinTrigger, LOW);
delayMicroseconds(2); 
digitalWrite(pinTrigger, HIGH);
delayMicroseconds(10); 
digitalWrite(pinTrigger, LOW);
durasi = pulseIn(pinEcho, HIGH);
jarak = (durasi/2) / 29.1;
jarak_pakan = (19.5 - jarak);
if(jarak_pakan<0)
{
  jarak_pakan=0;
}
volume=3.14*pow(7.9,2)*jarak_pakan;
sisa_pakan=0.53*volume;
Serial.print("Tinggi:");
Serial.println(jarak);
Serial.print("Tinggi Pakan :");
Serial.println(jarak_pakan);
Serial.print("Berat_pakan :");
Serial.println(sisa_pakan);
Serial.println(" cm");
Firebase.setFloat("monitoring/sisa_pakan_dipenampung",sisa_pakan);
}

void load_cell()
{
// Tampilkan Berat load cell
scale.set_scale(faktor_kalibrasi);
berat = scale.get_units(), 4;
Serial.println(berat);
if (berat<0)
{
 berat_pakan=0; 
} 
else
{
  berat_pakan=berat;
}
Serial.print("Jumlah Berat: ");
Serial.print(berat_pakan);
Serial.print(" Gram");
Serial.println();
delay(1000);
Firebase.setFloat("monitoring/berat_pakan_dipiring",berat_pakan); 
}

void loop() {
// Ambil Data Firebase
String path ="/";
FirebaseObject object = Firebase.get(path);
// Data pakan dikonversikan kan tipe float 
seporsi = object.getString("data_kucing/porsi_sekali").substring(1,6).toFloat();
jadwal1=object.getString("data_kucing/jadwal_makan1");
jadwal2=object.getString("data_kucing/jadwal_makan2");
jadwal3=object.getString("data_kucing/jadwal_makan3");
jadwal4=object.getString("data_kucing/jadwal_makan4");
//Pisahkan data jam dan menit jadwal lalu konversikan ke tipe integer 
jam1=jadwal1.substring(1,3).toInt();
menit1=jadwal1.substring(4,6).toInt();
jam2=jadwal2.substring(1,3).toInt();
menit2=jadwal2.substring(4,6).toInt();
// Beri nilai 25 jika jadwal makan 3 dan 4 kosong
if(jadwal3=="\"0\""){ 
}
else
{
  jam3=jadwal3.substring(1,3).toInt();
  menit3=jadwal3.substring(4,6).toInt();
}
if(jadwal4=="\"0\""){
  jam4=25;
}
else
{
  jam4=jadwal4.substring(1,3).toInt();
  menit4=jadwal4.substring(4,6).toInt();
}
int detik =6;
Serial.print("Waktu:");
// Menampilkan nilai waktu pada NTP
timeClient.update();
jam_ntp=timeClient.getHours();
menit_ntp=timeClient.getMinutes();
detik_ntp=timeClient.getSeconds();
Serial.print(jam_ntp);
Serial.print(":");
Serial.print(menit_ntp);
Serial.print(":");
Serial.println(detik_ntp);
ultrasonic();
delay(1000);
load_cell();  
  if ((jam_ntp==jam1 && menit_ntp==menit1 && detik_ntp<= detik )
  ||(jam_ntp== jam2 &&  menit_ntp==menit2 && detik_ntp<= detik )
  ||(jam_ntp== jam3 && menit_ntp==menit3 && detik_ntp<= detik )
  ||(jam_ntp== jam4 && menit_ntp== menit4 && detik_ntp<= detik ))
  {
// hitung takaran pakan 
Serial.println(berat_pakan);
takaran=seporsi+berat_pakan;
Serial.println(takaran);
int buff = 0;
while (buff != 1)
  {  
  scale.set_scale(faktor_kalibrasi);
  berat_pakan = scale.get_units(), 4;
  Serial.println(berat_pakan);
  if(berat_pakan>takaran)
  {
  Serial.println(berat_pakan);
  terkecil=berat_pakan;
 url ="http://maker.ifttt.com/trigger/beri_pakan/with/key/drXuIytMxgQxMMY4AUOaZ9?value1="+ String(seporsi)+"&value2="+String(berat_pakan);
 while (get_http(String("Pakan Dimakan")) != 0); 
  buff=1;
  }
  else if (takaran-berat_pakan<4)
  {
  myservo.write(115);
  delay(200);//300
  myservo.write(90);
  delay(2000);
  }
  else 
  {
  myservo.write(125);
  delay(250);//300
  myservo.write(90);
  delay(2000);
  }
  yield();
  }
 }
float selisih_berat = terkecil-berat_pakan;
if(selisih_berat>3)
{
 Serial.println("pakan sudah dimakan");
 url ="http://maker.ifttt.com/trigger/pakan/with/key/drXuIytMxgQxMMY4AUOaZ9?value1="+ String(selisih_berat)+"&value2="+String(berat_pakan);
 while (get_http(String("Pakan Dimakan")) != 0);   
 terkecil=berat_pakan;
}
if(sisa_pakan<kecil_pakan)
{
  kecil_pakan=sisa_pakan;
   url ="http://maker.ifttt.com/trigger/pakan_sedikit/with/key/drXuIytMxgQxMMY4AUOaZ9?value1="+ String(sisa_pakan);
   while (get_http(String("Pakan Sedikit")) != 0);   
}
else if (sisa_pakan>200)
{
kecil_pakan=200;
}
Serial.println(kecil_pakan);
}
 
int get_http(String state)
{
   HTTPClient http;
   int ret = 0;
   Serial.print("[HTTP] begin...\n");   
    http.begin(url);
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    // httpCode will be negative on error
    if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET code: %d\n", httpCode);

      if(httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
        ret = -1;
        Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        delay(500); // wait for half sec before retry again
    }
    http.end();
    return ret;
}  
