#ifndef	_LIB_HSTUB_H_
#define	_LIB_HSTUB_H_


typedef	int (*hstub_new_obj_fn)(void *hcookie, obj_data_t *odata, int vno);
typedef	void (*hstub_log_data_fn)(void *hcookie, char *data, int len, int dev);


typedef struct {
	hstub_new_obj_fn		new_obj_cb;
	hstub_log_data_fn		log_data_cb;
} hstub_cb_args_t;


/*
 * This structure keeps track of the state associated with each
 * of the storage devices.
 */

extern void *	device_init(int id, char *devid, void *hcookie, 
			hstub_cb_args_t *cbs);
extern int device_stop(void *dev, int id);
extern int device_terminate(void *dev, int id);
extern int device_start(void *dev, int id);
extern int device_set_searchlet(void *dev, int id, char *filter, char *spec);
extern int device_characteristics(void *handle, device_char_t *dev_chars);

#endif	/* _LIB_HSTUB_H_ */


