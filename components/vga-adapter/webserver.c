#include "zlib.h"
#include "globalvars.h"

static uint8_t *imageData = NULL;
static uint8_t *imagePtr = NULL;
static uint8_t *rawData = NULL;
static uint8_t *rawPtr = NULL;

httpd_handle_t web_server = NULL;

void putByte(uint8_t i)
{
	*imagePtr = i;
	imagePtr++;
}

void putDwordM(uint32_t i)
{
	putByte((i>>24) & 0xff);
	putByte((i>>16) & 0xff);
	putByte((i>>8) & 0xff);
	putByte(i & 0xff);
}

static esp_err_t png_get_handler(httpd_req_t *req)
{
    uint8_t col[32];

    imagePtr = imageData;
	// PNG Signatur
	putByte(0x89);
	putByte(0x50);
	putByte(0x4e);
	putByte(0x47);
	putByte(0x0d);
	putByte(0x0a);
	putByte(0x1a);
	putByte(0x0a);

	// Imageheader
	putDwordM(13);
	putByte(0x49);
	putByte(0x48);
	putByte(0x44);
	putByte(0x52);
	putDwordM(ABG_XRes);
	putDwordM(ABG_YRes);
	putByte(0x02);
	putByte(0x03);
	putByte(0x00);
	putByte(0x00);
	putByte(0x00);
	putDwordM(crc32(0, imagePtr-17,17));

	// Palette
	putDwordM(12);
	putByte(0x50);
	putByte(0x4c);
	putByte(0x54);
	putByte(0x45);
	for (int i=0;i<4;i++)
	{
        putByte((Current_Colors[i] << 2) & 0xc0); // red
        putByte((Current_Colors[i] << 4) & 0xc0); // green
        putByte((Current_Colors[i] << 6) & 0xc0); // blue
        col[Current_Colors[i] & 31] = i;
	}
	putDwordM(crc32(0, imagePtr-16,16));

    // Image Data header
    putDwordM(0);
    putByte(0x49);  // "IDAT"
    putByte(0x44);
    putByte(0x41);
    putByte(0x54);

    // VGA-Buffer + PNG-Tag in ZLIB-Puffer kopieren
    rawPtr = rawData;
    uint8_t d = 0;
    for (uint32_t y=0;y < ABG_YRes; y++)
    {
        uint8_t *img = VGA_BUF[y];
        *rawPtr = 0; // Row-Filter: none
        rawPtr++;
        for (uint32_t x=0; x<ABG_XRes; x++)	
        {
            d = (d << 2) | col[*img & 31];  // 2bit Pixel in bytes packen
            img++;
            if ((x & 3) == 3)
            {
                *rawPtr = d;
                rawPtr++;
            }
        }
    }

    // ZLIB-Kompression
    uint32_t packlen = 0x40000;
    compress2(imagePtr, &packlen, rawData, (ABG_XRes * ABG_YRes) / 4 + ABG_YRes,  1);
    imagePtr -= 8;
    putDwordM(packlen);
    imagePtr = imagePtr+packlen+4;
    putDwordM(crc32(0, imagePtr-4-packlen,packlen+4));

    // PNG-Ende
	putByte(0x00);
    putByte(0x00);
    putByte(0x00);
	putByte(0x00);
	putByte(0x49);
	putByte(0x45);
    putByte(0x4e);
    putByte(0x44);
    putByte(0xae);
    putByte(0x42);
    putByte(0x60);
    putByte(0x82);

    // PNG senden
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send_chunk(req, (char*)imageData, packlen+81);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

/* An HTTP GET handler */
static esp_err_t mainpage_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><html><head></head><body><img src=\"image.png\" height=\"800\" /></body></html>");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static const httpd_uri_t pngimage = 
{
    .uri       = "/image.png",
    .method    = HTTP_GET,
    .handler   = png_get_handler,
};

static const httpd_uri_t mainpage = 
{
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = mainpage_get_handler,
};

// Webserver starten
void start_webserver()
{
	if (imageData==NULL)
	{
		imageData = heap_caps_malloc(0x40000, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
	}
	if (rawData==NULL)
	{
		rawData = heap_caps_malloc(0x40000, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
	}
	if (web_server==NULL)
	{
		httpd_config_t config = HTTPD_DEFAULT_CONFIG();
		config.lru_purge_enable = true;
		config.core_id = 0;
	
		if (httpd_start(&web_server, &config) != ESP_OK) 
		{
			web_server = NULL;
			return;
		}
		httpd_register_uri_handler(web_server, &mainpage);
		httpd_register_uri_handler(web_server, &pngimage);
	}
}

void stop_webserver()
{
	if (imageData != NULL)
	{
		free(imageData);
		imageData = NULL;
	}
	if (rawData != NULL)
	{
		free(rawData);
		rawData = NULL;
	}
	if (web_server != NULL)
	{
		httpd_stop(web_server);
		web_server = NULL;
	}
}