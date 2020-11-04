
#include <SoftwareSerial.h>

unsigned char rising;			//送受信を決めるピン
unsigned char packet[32];       //送信パケット
unsigned char read_packet[32];  //受信パケット
unsigned char read_count = 0;   //受信パケットのためのカウント
unsigned char count = 0;        //送信パケットのためのカウント

SoftwareSerial mySerial(5, 6);	//SoftwareSerialのピン設定

void setup() {
  begin(7, 115200);
  torque(0x01, 0x01);
}

void loop() {
  int i;
  i = write(0x01, 0x1e, 0x01, 0x01);
  i = angle(0x01, 0x00);
  i = time(0x01, 0x00);
  i = positipn(0x01, 0x00, 0x00);
  i = read(0x01, 0x1e, 0x01);

}

//リターンパケットを得る関数
void Return(unsigned char stop)
{
  delay(15); //16バイトが9600bpsで送られてきた時に待つ時間(ms) = (16*8*10^3)/9600 = 13.3...
  if (mySerial.available())             //受信データがある
  {
    while (mySerial.available())
    {
      read_packet[read_count++] = mySerial.read();//データを得る
      if (read_count == stop) break;
    }
  }
}


//すべてのパケットとカウントをクリアする関数
void clear_packet()
{
  for (int i = 0; i < read_count && i < count; i++)
  {
    packet[i] = 0;
    read_packet[i] = 0;
  }

  delayMicroseconds(100);
  count = 0;        //countを0にする
  read_count = 0;   //read_countを0にする
}


//チェックサム関数
unsigned char check_sum()
{
  unsigned char sum = 0;
  for (int i = 2; i < count; i++)
  {
    sum = sum ^ packet[i];
  }

  return sum;
}


//リターンパケットのチェックサム関数
unsigned char check_rsum()
{
  unsigned char rsum = 0;
  for (int i = 2; i < read_count; i++)
  {
    rsum = rsum ^ read_packet[i];
  }

  return rsum;
}


//パケットを送信する関数
void send_packet()
{
  digitalWrite(rising, HIGH);		//送信モード
  mySerial.write(packet, count);		//パケットを送信
  delayMicroseconds(100);				//送信待ち
  digitalWrite(rising, LOW);			//受信モード
}





//SoftwareSerialの通信を開始する,送受信を決めるピンを割り当てる関数
void begin(unsigned char rising_pin, unsigned long bps)
{
  mySerial.begin(bps);                        //SoftwareSerialの通信速度設定
  rising = rising_pin;                        //送受信ピン設定
  pinMode(rising, OUTPUT);
}


//トルクを設定する関数
unsigned char torque(unsigned char id, unsigned char torque)
{
  unsigned char rsum = 0;
  clear_packet();						//送受信パケットを共にクリアする

  packet[count] = 0xFA;				//ヘッダ
  count++;
  packet[count] = 0xAF;				//ヘッダ
  count++;
  packet[count] = id;					//ID
  count++;
  packet[count] = 0x01;				//フラッグ　ACK/NACKリターン要求
  count++;
  packet[count] = 0x24;				//トルクのアドレス
  count++;
  packet[count] = 0x01;				//データブロックの長さ
  count++;
  packet[count] = 0x01;				//サーボの個数　ショートパケットでメモリーマップに書き込む時は常に0x01
  count++;
  packet[count] = torque;				//0x01でトルクON、0x00でトルクOFF
  count++;
  packet[count] = check_sum();		//チェックサム作成

  send_packet();						//パケット送信

  Return(8);									//パケット受信 8バイト
  rsum = check_rsum();						//リターンパケットのチェックサム
  if (rsum != read_packet[read_count]) return 2;//リターンパケットのチェックサムが異なるなら

  switch (read_packet[7])						//リターンパケットの値からACK/NACK判定
  {
    case 0x07: return 1; break;
    case 0x08: return 0; break;
    default  : return 2; break;
  }
}


//特定のアドレスに書き込む関数 2バイトまで
unsigned char write(unsigned char id, unsigned char address, unsigned char lengs, int data)
{
  if (lengs != 1 | lengs != 2) return 2;
  unsigned char rsum = 0;
  clear_packet();						//送受信パケットを共にクリアする

  packet[count] = 0xFA;				//ヘッダ
  count++;
  packet[count] = 0xAF;				//ヘッダ
  count++;
  packet[count] = id;					//ID
  count++;
  packet[count] = 0x01;				//フラッグ　ACK/NACKリターン要求
  count++;
  packet[count] = address;			//アドレス
  count++;
  packet[count] = lengs;				//データブロックの長さ
  count++;
  packet[count] = 0x01;				//サーボの個数　ショートパケットでメモリーマップに書き込む時は常に0x01
  count++;

  if (lengs == 1)
  {
    packet[count] = 0x00ff & data;	//data
  }
  else if (lengs == 2)
  {
    packet[count] = 0x00ff & data;	//data
    count++;
    packet[count] = 0x00ff & (data >> 8);
  }
  else {
    return 2;
  }

  count++;
  packet[count] = check_sum();		//チェックサム作成

  send_packet();						//パケット送信

  Return(8);									//パケット受信 8バイト
  rsum = check_rsum();						//リターンパケットのチェックサム
  if (rsum != read_packet[read_count]) return 2;//リターンパケットのチェックサムが異なるなら

  switch (read_packet[7])						//リターンパケットの値からACK/NACK判定
  {
    case 0x07: return 1; break;
    case 0x08: return 0; break;
    default: return 2; break;
  }
}


//角度を命令をする関数
unsigned char angle(unsigned char id, int angle)
{
  unsigned char rsum = 0;
  clear_packet();      //送受信パケットを共にクリアする

  packet[count] = 0xFA;
  count++;
  packet[count] = 0xAF;
  count++;
  packet[count] = id;                        //servoのID
  count++;
  packet[count] = 0x01;                   //受信を決めるフラッグ
  count++;
  packet[count] = 0x1E;                   //メモリーマップ
  count++;
  packet[count] = 0x02;                   //長さ
  count++;
  packet[count] = 0x01;                   //Count ショートパケットでは常に0x01
  count++;
  packet[count] = 0x00ff & angle;   //angleの下位バイト 0x00FF & angle;
  count++;
  packet[count] = 0x00ff & (angle >> 8); //angleの上位バイト 0x00FF & (angle >> 8);
  count++;
  packet[count] = check_sum();     //チェックサム

  send_packet();

  Return(8);                                         //受信
  rsum = check_rsum();
  if (rsum != read_packet[read_count]) return 2;

  switch (read_packet[7])
  {
    case 0x07: return 1; break; //ACK
    case 0x08: return 0; break; //NACK
    default  : return 2; break;     //なぞの何かを受信
  }
}


//時間の命令する関数
unsigned char time(unsigned char id, int time)
{
  unsigned char rsum = 0;
  clear_packet();    //送受信パケットを共にクリアする

  packet[count] = 0xFA;
  count++;
  packet[count] = 0xAF;
  count++;
  packet[count] = id;
  count++;
  packet[count] = 0x01;
  count++;
  packet[count] = 0x20;
  count++;
  packet[count] = 0x02;
  count++;
  packet[count] = 0x01;
  count++;
  packet[count] = 0x00FF & time;
  count++;
  packet[count] = 0x00FF & (time >> 8);
  count++;
  packet[count] = check_sum();

  send_packet();

  Return(8);                                     //受信
  rsum = check_rsum();
  if (rsum != read_packet[read_count]) return 2;

  switch (read_packet[7])
  {
    case 0x07: return 1; break;
    case 0x08: return 0; break;
    default  : return 2; break;
  }
}


//角度と移動時間の命令する関数
unsigned char positipn(unsigned char id, int angle, int time)
{
  unsigned char rsum = 0;
  clear_packet();    //送受信パケットを共にクリアする

  packet[count] = 0xFA;
  count++;
  packet[count] = 0xAF;
  count++;
  packet[count] = id;
  count++;
  packet[count] = 0x01;
  count++;
  packet[count] = 0x1E;
  count++;
  packet[count] = 0x04;
  count++;
  packet[count] = 0x01;
  count++;
  packet[count] = 0x00FF & angle;
  count++;
  packet[count] = 0x00FF & (angle >> 8);
  count++;
  packet[count] = 0x00FF & time;
  count++;
  packet[count] = 0x00FF & (time >> 8);
  count++;
  packet[count] = check_sum();

  send_packet();

  Return(8);      //受信
  rsum = check_rsum();
  if (rsum != read_packet[read_count]) return 2;

  switch (read_packet[7])
  {
    case 0x07: return 1; break;
    case 0x08: return 0; break;
    default  : return 2; break;
  }
}


//メモリを読む
int read(unsigned char id, unsigned char number, unsigned char leng)
{
  clear_packet();    //送受信パケットを共にクリアする

  int memory = 0;    //返す値の格納庫
  unsigned char rsum = 0;

  packet[count] = 0xFA;
  count++;
  packet[count] = 0xAF;
  count++;
  packet[count] = id;
  count++;
  packet[count] = 0x0f;
  count++;
  packet[count] = number;
  count++;
  packet[count] = 0x02;
  count++;
  packet[count] = 0x00;
  count++;
  packet[count] = check_sum();

  send_packet();

  Return(9);      //受信
  rsum = check_rsum();
  if (rsum != read_packet[read_count]) return 2;

  if (leng == 1)
  {
    memory = read_packet[7];
  }
  else if (leng == 2)
  {
    memory = ((read_packet[8] << 8) | (read_packet[7]));
  }
  else {
    return 2;
  }

  return memory;
}


