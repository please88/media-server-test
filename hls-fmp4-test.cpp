#include "hls-fmp4.h"
#include "hls-m3u8.h"
#include "hls-param.h"
#include "mov-format.h"
#include "flv-proto.h"
#include "flv-reader.h"
#include "flv-demuxer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct _flv_hls_fmp4_param{
    hls_fmp4_t* hls;
	int track_aac;
	int track_264;
}flv_hls_fmp4_param;

static char s_packet[2 * 1024 * 1024];

static int hls_init_segment(hls_fmp4_t* hls, hls_m3u8_t* m3u)
{
	int bytes = hls_fmp4_init_segment(hls, s_packet, sizeof(s_packet));

	FILE* fp = fopen("out-fmp4/0.mp4", "wb");
	fwrite(s_packet, 1, bytes, fp);
	fclose(fp);

	return hls_m3u8_set_x_map(m3u, "out-fmp4/0.mp4");
}

static int hls_segment(void* m3u8, const void* data, size_t bytes, int64_t /*pts*/, int64_t dts, int64_t duration)
{
	static int i = 0;
	static char name[128] = { 0 };
	snprintf(name, sizeof(name), "out-fmp4/%d.mp4", ++i);
	FILE* fp = fopen(name, "wb");
	fwrite(data, 1, bytes, fp);
	fclose(fp);

	return hls_m3u8_add((hls_m3u8_t*)m3u8, name, dts, duration, 0);
}

static int flv_handler(void* param, int codec, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags)
{
    flv_hls_fmp4_param* fmp4_param = (flv_hls_fmp4_param*)param;
	hls_fmp4_t* hls = fmp4_param->hls;
    int track_aac = fmp4_param->track_aac;
	int track_264 = fmp4_param->track_264;

	switch (codec)
	{
	case FLV_AUDIO_AAC:
		return hls_fmp4_input(hls, track_aac, data, bytes, pts, dts, 0);

	case FLV_VIDEO_H264:
		return hls_fmp4_input(hls, track_264, data, bytes, pts, dts, flags ? MOV_AV_FLAG_KEYFREAME : 0);

	default:
		// nothing to do
		return 0;
	}

}

void hls_segmenter_flv(const char* file)
{
	hls_m3u8_t* m3u = hls_m3u8_create(0, 7);
	hls_fmp4_t* hls = hls_fmp4_create(HLS_DURATION * 1000, hls_segment, m3u);

    flv_hls_fmp4_param* param = (flv_hls_fmp4_param*)malloc(sizeof(flv_hls_fmp4_param));
    param->hls = hls;
    param->track_aac = hls_fmp4_add_audio(hls, MOV_OBJECT_AAC, 1, 16, 8000, NULL, 0);
    param->track_264 = hls_fmp4_add_video(hls, MOV_OBJECT_H264, 1280, 720, NULL, 0);
    
	// write init segment
	hls_init_segment(hls, m3u);

	void* flv = flv_reader_create(file);
	flv_demuxer_t* demuxer = flv_demuxer_create(flv_handler, param);

	int r, type;
	uint32_t timestamp;
	static char data[2 * 1024 * 1024];
	while ((r = flv_reader_read(flv, &type, &timestamp, data, sizeof(data))) > 0)
	{
		flv_demuxer_input(demuxer, type, data, r, timestamp);
	}

	flv_demuxer_destroy(demuxer);
	flv_reader_destroy(flv);

    hls_fmp4_destroy(hls);

    // write m3u8 file
    hls_m3u8_playlist(m3u, 1, s_packet, sizeof(s_packet));
    hls_m3u8_destroy(m3u);

    FILE* fp = fopen("playlist_fmp4.m3u8", "wb");
    fwrite(s_packet, 1, strlen(s_packet), fp);
    fclose(fp);

    free(param);
}


int main(int argc, char* argv[])
{
    if(argc < 2){
        printf("input flv file\n");
        return -1;
    }

    hls_segmenter_flv(argv[1]);
    return 0;
}


