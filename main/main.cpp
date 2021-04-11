#include <client.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/sntp_opts.h"
#include <string.h>
#include <tcpsocket.h>

#include "lib/freertos_cpp/semaphore.h"
#include "lib/onewire/onewire.h"
#include "lib/onewire/ds18b20.h"
#include "lib/tft/tft.h"
#include "lib/jbvprotocol/client.h"
#include "lib/tcpip/tcplistener.h"
#include "settings.h"

TFT tft = TFT::Get_ILI9341();
JBVProtocol::Client client(SoftwareID::TestApp);
float tmp1 = 0;

extern "C" {
   void app_main();
}



esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}


class Settings : public BaseSettings
{
public:
	Setting<int> SomeVal = Setting<int>("test", 0, &settingsList);

	Settings() : BaseSettings("Global")
	{

	}
};




void Accepted(TCPSocket *sock)
{
	//ESP_LOGI("MAIN", "Accepted");
	client.SetConnection(sock);
}

void HandleFrame(JBVProtocol::Client *c, JBVProtocol::Frame *frame)
{
	int sep = -1;

	for(int i=0; i<frame->DataLength; i++)
	{
		if(frame->Data[i] == ' ')
		{
			sep = i;
			break;
		}
	}

	int len = sep;
	if(len == -1)
		len = frame->DataLength;

	char buf[frame->DataLength + 1];
	memcpy(buf, frame->Data, frame->DataLength);
	buf[frame->DataLength] = 0;

	if(strcmp(buf, "DTMP") == 0)
	{
		//Prepare reply.
		std::string reply = std::to_string(tmp1);
		JBVProtocol::Frame txFrame;
		txFrame.Sequence = frame->Sequence;
		txFrame.RxID = frame->TxID;
		txFrame.SetData(reply.c_str(), reply.length());
		c->SendFrame(&txFrame);

	}



}


void app_main(void)
{

	nvs_flash_init();
	tcpip_adapter_init();
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

	wifi_config_t sta_config = {};
	memcpy(sta_config.sta.ssid, "vanBassum", 10);
	memcpy(sta_config.sta.password, "pedaalemmerzak", 15);
	sta_config.sta.bssid_set = false;
	ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
	ESP_ERROR_CHECK( esp_wifi_start() );
	ESP_ERROR_CHECK( esp_wifi_connect() );

	setenv("TZ", "UTC-1UTC,M3.5.0,M10.5.0/3", 1);
	tzset();
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();


	//vTaskDelay(5000/portTICK_PERIOD_MS);

	TCPListener listener;
	listener.OnSocketAccepted.Bind(&Accepted);

	client.HandleFrame.Bind(&HandleFrame);

/*


	TCPSocket con;
	con.Connect("192.168.11.14", 1000, true);
	client.SetConnection(&con);



	while(1)
		vTaskDelay(1000/portTICK_PERIOD_MS);

*/

	tft.Init();
	tft.FillScreen(Color(0, 0, 0));

	OneWire::Bus onewire(GPIO_NUM_13);
	OneWire::DS18B20 *tempSensors[10];

	Font font(ILGH32XB);
	tft.FillScreen(Color(0, 0, 0));


	while(1)
	{
		int devCnt = onewire.Search((OneWire::Device **)tempSensors, 10, OneWire::Devices::DS18B20);

		int i;
		for(i=0; i<devCnt; i++)
			tempSensors[i]->StartReading();

		vTaskDelay(750/portTICK_PERIOD_MS);
		int h = 34;

		for(i=0; i<devCnt; i++)
		{
			char buf[32];
			int y = i * h;
			float t = tempSensors[i]->GetReading();
			if (i ==0 )
				tmp1 = t;

			snprintf(buf, 32, "T%d = %.02f C", i, t);
			tft.DrawFillRect(0, y, 240, y + h, Color(0, 0, 0));
			tft.DrawString(&font, 5, y + h, buf, Color(255, 255, 255));

			delete tempSensors[i];
		}
	}
}










