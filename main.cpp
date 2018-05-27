#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include <opencv2/opencv.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libpostproc/postprocess.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/rational.h>
} // end extern "C".

using namespace std;
using namespace cv;

void saveYUV(unsigned char *buf,int wrap, int xsize,int ysize,char *filename);

int main() {
    
    AVCodec* pCodec;
    AVCodecContext* pAVCodecContext;
    SwsContext* pSWSContext;
    AVFormatContext* pAVFormatContext;
    AVFrame* pAVFrame;
    AVFrame* pAVFrameBGR;
    AVPacket AVPacket;
    std::string sFile;
    uint8_t* puiBuffer;
    int iResult;
    int iFrameCount = 0;
    int iGotVideo;
    //int iGotAudio;
    int iDecodedBytesVideo;
    //int iDecodedBytesAudio;
    int iVideoStreamIdx = -1;
    //int iAudioStreamIdx = -1;
    int iNumBytes;
    double fps;
    Mat cvmImage;
    
    pCodec = NULL;
    pAVCodecContext = NULL;
    pSWSContext = NULL;
    pAVFormatContext = NULL;
    pAVFrame = NULL;
    pAVFrameBGR = NULL;
    puiBuffer = NULL;

    const char *in_filename = "video.mp4";
    
    av_register_all();
    
    if((iResult = avformat_open_input(&pAVFormatContext, in_filename, NULL, NULL)) < 0){
        printf("Cannot open input file!\n");
        return -1;
    }
    
    if((iResult = avformat_find_stream_info(pAVFormatContext, NULL)) < 0){
        printf("Fail to retrieve input stream information!\n");
        return -1;
    }
    
    // AVRational timebase = pAVFormatContext->streams[0]->time_base;
    int64_t seek_pos = (int64_t)(1*AV_TIME_BASE);
    av_seek_frame(pAVFormatContext, -1, seek_pos, AVSEEK_FLAG_BACKWARD);
    avformat_flush(pAVFormatContext);

    for (int i = 0; i < pAVFormatContext->nb_streams; i++)
    {
        if (pAVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            iVideoStreamIdx = i;    //iVideoStreamIdx = 0
            //cout << iVideoStreamIdx << endl;
            
            pAVCodecContext = pAVFormatContext->streams[iVideoStreamIdx]->codec;
            
            if (pAVCodecContext)
            {
                pCodec = avcodec_find_decoder(pAVCodecContext->codec_id);
            }
            
            // Print fps
            fps = av_q2d(pAVFormatContext->streams[iVideoStreamIdx]->r_frame_rate);
            printf("Frame Rate = %.2f\n", fps);
            
            //break;
        }
    }
    
    if (pCodec && pAVCodecContext)
    {
        if((iResult = avcodec_open2(pAVCodecContext, pCodec, NULL)) < 0){
            printf("Cannot open codec\n");
            return -1;        }
        
        pAVFrame = av_frame_alloc();
        pAVFrameBGR = av_frame_alloc();
        
        iNumBytes = avpicture_get_size(AV_PIX_FMT_BGR24, pAVCodecContext->width, pAVCodecContext->height);
        
        // For getting yuv420 file
        // iNumBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pAVCodecContext->width, pAVCodecContext->height);
        
        puiBuffer = (uint8_t *)av_malloc(iNumBytes*sizeof(uint8_t));
        
        avpicture_fill((AVPicture *)pAVFrameBGR, puiBuffer, AV_PIX_FMT_RGB24, pAVCodecContext->width, pAVCodecContext->height);

        pSWSContext = sws_getContext(pAVCodecContext->width, pAVCodecContext->height, pAVCodecContext->pix_fmt, pAVCodecContext->width, pAVCodecContext->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

        
        while (av_read_frame(pAVFormatContext, &AVPacket) >= 0)
        {
            if (AVPacket.stream_index == iVideoStreamIdx)
            {
                //Get video info
                iDecodedBytesVideo = avcodec_decode_video2(pAVCodecContext, pAVFrame, &iGotVideo, &AVPacket);
                
                if (iGotVideo)
                {
                    // For getting yuv420 file
                    saveYUV(pAVFrame->data[0], pAVFrame->linesize[0], pAVCodecContext->width, pAVCodecContext->height, "output.yuv");       //Y
                    saveYUV(pAVFrame->data[1], pAVFrame->linesize[1], pAVCodecContext->width/2, pAVCodecContext->height/2, "output.yuv");   //U
                    saveYUV(pAVFrame->data[2], pAVFrame->linesize[2], pAVCodecContext->width/2, pAVCodecContext->height/2, "output.yuv");   //V
                    
                    if (pSWSContext)
                    {
                        iResult = sws_scale(pSWSContext, pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContext->height, pAVFrameBGR->data, pAVFrameBGR->linesize);

                        if (iResult > 0)
                        {
                            cvmImage = Mat(pAVFrame->height, pAVFrame->width, CV_8UC3,  pAVFrameBGR->data[0], pAVFrameBGR->linesize[0]);

                            if (cvmImage.empty() == false)
                            {
                                imwrite("Frames/" + std::to_string(iFrameCount) + ".jpg", cvmImage);
                                imshow("image", cvmImage);
                                waitKey(1);
                            }
                        }
                    }

                    iFrameCount++;
                    
                }
            }
        }
    }
    
    avformat_close_input(&pAVFormatContext);
    
    return 0;
}

void saveYUV(unsigned char *buf,int wrap, int xsize,int ysize,char *filename)
{
    FILE *f;
    int i;
    
    f=fopen(filename,"ab+");
    
    for(i=0;i<ysize;i++)
    {
        fwrite(buf + i * wrap, 1, xsize, f );
    }
    fclose(f);
}

