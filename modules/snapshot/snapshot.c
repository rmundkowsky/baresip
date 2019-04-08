/**
 * @file snapshot.c  Snapshot Video-Filter
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "png_vf.h"
#include <unistd.h>

#include <math.h>
#include <sys/time.h>
#include "sendfilename.h"


/**
 * @defgroup snapshot snapshot
 *
 * Take snapshot of the video stream and save it as PNG-files
 *
 *
 * Commands:
 *
 \verbatim
 snapshot       Take video snapshot
 \endverbatim
 */


static struct timeval last = {.tv_sec=0, .tv_usec=0};
static int frameRatePerSec=1.0;
static bool continous_snapshot = true;


static bool flag_enc, flag_dec;

static const char* peername=NULL;
static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg);

static int encode(struct vidfilt_enc_st *st, struct vidframe *frame,
		  uint64_t *timestamp)
{
	(void)st;
	(void)timestamp;

	if(1==2){
	  if (!frame)
	    return 0;
	  
	  if (flag_enc) {
	    flag_enc = false;
	    png_save_vidframe(frame, "snapshot-send");
	  }
	}
	return 0;
}


static int decode(struct vidfilt_dec_st *st, struct vidframe *frame,
		  uint64_t *timestamp)
{
        (void)st;
	(void)timestamp;

	char* file_prefix=NULL;
	struct timeval current;
	float diffInMilliSecs;
	float millisecPerFrame = 1.0/(frameRatePerSec/1000.0);

	if(last.tv_sec == 0 && last.tv_usec == 0){
	  gettimeofday(&last, NULL);
	}

	gettimeofday(&current, NULL);

	diffInMilliSecs = ceil(current.tv_sec*1000 + current.tv_usec / 1000 -
			       last.tv_sec*1000 - last.tv_usec / 1000);

	if(diffInMilliSecs < millisecPerFrame){
	  /*              info("no decode snapshot");
                info(" diff %f\n",diffInSecs);
                info(" rate %d\n", frameRatePerSec);
	  */
	  return 0;
	}
	else{
	  last = current;
	}
	
	
	if (!frame)
		return 0;

	if (continous_snapshot || flag_dec) {
	//	if (flag_dec) {
		flag_dec = false;

		if(peername){
		  file_prefix = peername;
		}
		else{
		  file_prefix = "snapshot-recv";
		}
		
		png_save_vidframe(frame, file_prefix);
	}

	return 0;
}


static int do_snapshot(struct re_printf *pf, void *arg)
{
	(void)pf;
	(void)arg;

	/* NOTE: not re-entrant */
	flag_enc = flag_dec = true;

	return 0;
}


static struct vidfilt snapshot = {
	LE_INIT, "snapshot", NULL, encode, NULL, decode,
};


static const struct cmd cmdv[] = {
	{"snapshot", 0, 0, "Take video snapshot", do_snapshot },
};




static int module_init(void)
{
        int err;
	
	err = uag_event_register(ua_event_handler, 0);

	if (err)

	  return err;

	vidfilt_register(baresip_vidfiltl(), &snapshot);
	return cmd_register(baresip_commands(), cmdv, ARRAY_SIZE(cmdv));
}


static int module_close(void)
{
  uag_event_unregister(ua_event_handler);
  vidfilt_unregister(&snapshot);
  cmd_unregister(baresip_commands(), cmdv);
  return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(snapshot) = {
	"snapshot",
	"vidfilt",
	module_init,
	module_close
};


static void ua_event_handler(struct ua *ua, enum ua_event ev,
			     struct call *call, const char *prm, void *arg)
{


  (void)prm;
  (void)arg;

  switch (ev) {

  case UA_EVENT_CALL_ESTABLISHED:
    //UA_EVENT_CALL_INCOMING:

    peername = call_peername(call);

    debug("echo: CALL_INCOMING: peer=%s  -->  local=%s %s\n",
	  call_peeruri(call),
	  call_localuri(call),
	  peername);


    break;



  default:
    break;
  }

}

