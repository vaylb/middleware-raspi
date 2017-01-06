
#include "OMXH264Player.h"
#include <QDebug>

OMXH264Player::OMXH264Player(QObject *parent):
    QObject(parent),
    video_decode(NULL),
    video_scheduler(NULL),
    video_render(NULL),
    clock(NULL),
    port_settings_changed(0),
    first_packet(1),
    omxbuf(NULL)
{
    qDebug()<<"OMXH264Player constructor start";
    bcm_host_init();
    int status = 0;
    memset(list, 0, sizeof(list));
    memset(tunnel, 0, sizeof(tunnel));

    if((client = ilclient_init()) == NULL){
        qDebug()<<"OMXH264Player ilclient_init error";
        return;
    }
    OMX_ERRORTYPE error;
    if((error = OMX_Init()) != OMX_ErrorNone){
        ilclient_destroy(client);
        qDebug("OMXH264Player OMX_Init error %x",error);
        return;
    }

    // create video_decode
    if(ilclient_create_component(client, &video_decode, "video_decode", (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0)
      status = -14;
    list[0] = video_decode;

    // create video_render
    if(status == 0 && ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
    list[1] = video_render;

    // create clock
    if(status == 0 && ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
    list[2] = clock;

    memset(&cstate, 0, sizeof(cstate));
    cstate.nSize = sizeof(cstate);
    cstate.nVersion.nVersion = OMX_VERSION;
    cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
    cstate.nWaitMask = 1;
    if(clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
      status = -13;

    // create video_scheduler
    if(status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
    list[3] = video_scheduler;

    set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
    set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90);
    set_tunnel(tunnel+2, clock, 80, video_scheduler, 12);

    // setup clock tunnel first
    if(status == 0 && ilclient_setup_tunnel(tunnel+2, 0, 0) != 0)
      status = -15;
    else
      ilclient_change_component_state(clock, OMX_StateExecuting);

    if(status == 0)
      ilclient_change_component_state(video_decode, OMX_StateIdle);

    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    format.nVersion.nVersion = OMX_VERSION;
    format.nPortIndex = 130;
    format.eCompressionFormat = OMX_VIDEO_CodingAVC;

    if(status == 0 &&
          OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
          ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0){
        qDebug()<<"OMXH264Player constructor success";
    }else{
        qDebug()<<"OMXH264Player constructor error, status = "<<status;
    }
    ilclient_change_component_state(video_decode, OMX_StateExecuting);
}

OMXH264Player::~OMXH264Player(){
    ilclient_disable_tunnel(tunnel);
    ilclient_disable_tunnel(tunnel+1);
    ilclient_disable_tunnel(tunnel+2);
    ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
    ilclient_teardown_tunnels(tunnel);

    ilclient_state_transition(list, OMX_StateIdle);
    ilclient_state_transition(list, OMX_StateLoaded);

    ilclient_cleanup_components(list);

    OMX_Deinit();

    ilclient_destroy(client);
}

void OMXH264Player::draw( unsigned char * image, int size){
    unsigned int data_len = 0;
    omxbuf = ilclient_get_input_buffer(video_decode, 130, 1);
    unsigned char *dest = omxbuf->pBuffer;
    memcpy(dest, image, size);
    data_len = size;
    qDebug()<<"write "<<data_len;

    if(port_settings_changed == 0 && (data_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0)){
        port_settings_changed = 1;
        if(ilclient_setup_tunnel(tunnel, 0, 0) != 0) return;
        ilclient_change_component_state(video_scheduler, OMX_StateExecuting);
        // now setup tunnel to video_render
        if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0) return ;
        ilclient_change_component_state(video_render, OMX_StateExecuting);
    }
    if(!data_len) return ;
    omxbuf->nFilledLen = data_len;
    omxbuf->nOffset = 0;
    if(first_packet)
    {
        omxbuf->nFlags = OMX_BUFFERFLAG_STARTTIME;
        first_packet = 0;
    }else omxbuf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

    OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), omxbuf);
}

void OMXH264Player::playbackTest(){
    unsigned char* buffer = (unsigned char*)malloc(81920);
    memset(buffer, 0, 81920);
    FILE *in;
    if((in = fopen("/home/pi/middleware/FaceTimeEveryDay.264", "rb")) == NULL)
        return;
    int read = 0;
    while((read = fread(buffer, 1, 200, in))>0){
        draw(buffer, read);
    }
    clear();
}

void OMXH264Player::clear(){
    if((omxbuf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL){
        omxbuf->nFilledLen = 0;
        omxbuf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;
        if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), omxbuf) != OMX_ErrorNone) qDebug()<<"EmptyThisBuffer error";

        // wait for EOS from render
        ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0, ILCLIENT_BUFFER_FLAG_EOS, -1);

        // need to flush the renderer to allow video_decode to disable its input port
        ilclient_flush_tunnels(tunnel, 0);
    }
}
