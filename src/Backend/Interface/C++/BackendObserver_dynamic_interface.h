#ifndef BACKEND_OBSERVER_DYNAMIC_INTERFACE_H
#define BACKEND_OBSERVER_DYNAMIC_INTERFACE_H

typedef int (*DBackend_RequestDataDelete_PROC_TYPE) (char* a_dataname);
typedef bool (*DBackend_OnDataOut_PROC_TYPE) (char* a_dataname, char* a_fit_width, char* a_display_type, char* a_align, char* a_grid);
typedef char* (*DBackend_OnRefreshDataWindow_PROC_TYPE) (char* a_dataname, bool a_full_change, bool a_first_change);
typedef char* (*DBackend_OnSystemExit_PROC_TYPE) ();
typedef char* (*DBackend_OnBindScrolling_PROC_TYPE) (const char* a_src_data_name, const char* a_dst_data_name, const char* a_remove_binding);

#endif /// BACKEND_OBSERVER_DYNAMIC_INTERFACE_H
