## Motor sürücü devresi hakkında genel notlar

  Mosfet sürücü entegresinin akım sınırı yeterli mi analiz et.

Vin kaynağı için voltaj regülatörü ayarlanacak. İstediğim 6-60V aralığı, şimdilik elimde olan lm294 6-26V regülatörü kullanacağım.

Gate drievr entegresinin besleme hattına paralel kolda bağlı olan seri kapasitör bypass işi görmektedir. Yüksek frekans darbeleri emer ve devredeki ani akım çekimlerinde voltajı sabit tutma işi görür.


--high side - low side sürme
--faz Akım, faz gerilim, zıt emk rpm, mosfet sıcaklığı, rejeneratif kazanç??, kart sıcaklık bilgilerini toplama;  trickle charge pump to support 100% duty cycle. Bunun için;
Mosfet sürücü seçme / Devre kurma

%100 duty cycle destekli bootstrap devresi nasıl kurulur.

DRV8302- 360TL


Mosfet Seçimi
IAUCN08S7L013ATMA1	En iyisi, 1mohm RDSon 300A 80V

$x=y$

### Çalışma Koşulları
$Vbat = 50,4V (12S pil)$

$Inormal = 30A$

$Watt = 1,5kW$

Sistem bu veriler gözönünde bulundurularak tasarlanmıştır.

Her zaman önce **pull** sonra **push**!

### Termal analiz 
https://jrainimo.com/build/2024/11/oss-thermal-simulation-of-pcbs/
Açık kaynaklı programları kullanarak sıcaklık analizi yapılabilir.

### Board Tasarımında Dikkat Edilecek Hususlar
(https://www.youtube.com/watch?v=XKBNmYKVMiE)

Ringing ölçümü için alan bıraktığından emin ol. Osiloskop bağlayıp sinyali gözlemlemen gerekebilir.

Gronud Bounce??

Cross Talk: Birbiri üstünde gerilim indükleyerek yollardaki sinyalde bozulmaya yol açılmasıdır. Anahtarlama yolları sinyal yollarına yakın olmamalıdır.

HV ve LV bölgeleri birbirinden ayır. GND hatlarını da gnd plane üstünde birleştir.

Gate drive yol uzunluğunu azalt, indüktans azalsın. 
Yol genişliğini arttır, empedans azalsın. En iyisi, düşük empedans.
Via kullanmaktan kaçın mosfet Gate pininde.
Simetrik (Dif.) GH ve GL yolları çiz. Sinyaller aynı anda mosfetlere varsın. 
Gate drive yolları altına gnd plane atın.
GH GL dirençlerini mosfetlere yakın yerleştir ki mosfet sürücüyü ısıtmasın.

Birden fazla katmanda GND plane kullan ve vialarla geçişler yap ki gnd empedansı azalsın.

Yol uzunlukları ve via sayısını minimize et.

CURRENT SENSE: 
  Şönt direnç yollarını ground kalkanlama ve via ile çerçeveleme ile EMI'dan koru.