#include "helper.h"

int init(CURLM* cm, CSTACK* frontier, CSTACK* visited, int t) {
    int links = 0;
    while(1){
        if(links == t){
            return 1;
        }
        char* url_front = NULL;
        pop(frontier, &url_front);
        //printf("Init URL_FRONT: %s\n", url_front);
        if(url_front == NULL){
            if(links == 0){
                return 0;
            } else {
                return 1;
            }
        }
        else {
            ENTRY e, *ep;
            e.key = url_front;
            ep = hsearch(e, FIND);
            if(ep == NULL){
                ep = hsearch(e, ENTER);
                push(visited, url_front);
                //printf("Visited URL: %s\n", url_front);
                
                RECV_BUF* p_recv_buf = malloc(sizeof(RECV_BUF));
                CURL* eh = easy_handle_init(p_recv_buf, url_front);

                curl_multi_add_handle(cm, eh);
                links++;
            }
            else {
                free(url_front);
            }
        }
    }
}

int run(CSTACK* frontier, CSTACK* visited, CSTACK* PNGURL, int t, int m) {
    CURLM* cm = NULL;
    CURL* eh = NULL;
    CURLMsg* msg = NULL;
    CURLcode return_code = 0;
    int still_running = 0, msgs_left = 0;

    curl_global_init(CURL_GLOBAL_ALL);

    while(1) {
        cm = curl_multi_init();
        if(pngs >= m){
            curl_multi_cleanup(cm);
            curl_global_cleanup();
            return 0;
        }

        if (!init(cm, frontier, visited, t)){
            curl_multi_cleanup(cm);
            curl_global_cleanup();
            return 0;
        }

        still_running = 0;

        curl_multi_perform(cm, &still_running);

        do {
            int numfds=0;
            int res = curl_multi_wait(cm, NULL, 0, MAX_WAIT_MSECS, &numfds);
            if(res != CURLM_OK) {
                fprintf(stderr, "error: curl_multi_wait() returned %d\n", res);
                //return EXIT_FAILURE;
            }
            curl_multi_perform(cm, &still_running);

        } while(still_running);

         while ((msg = curl_multi_info_read(cm, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                eh = msg->easy_handle;

                return_code = msg->data.result;
                RECV_BUF *recv_buf;

                if(return_code!=CURLE_OK) {
                    fprintf(stderr, "CURL error code: %d\n", msg->data.result);
                    curl_easy_getinfo(eh, CURLINFO_PRIVATE, &recv_buf);
                    recv_buf_cleanup(recv_buf);
                    free(recv_buf);
                    curl_multi_remove_handle(cm, eh);
                    curl_easy_cleanup(eh);
                    continue;
                }

                curl_easy_getinfo(eh, CURLINFO_PRIVATE, &recv_buf);

                process_data(eh, recv_buf, frontier, PNGURL);

                curl_multi_remove_handle(cm, eh);
                curl_easy_cleanup(eh);
                recv_buf_cleanup(recv_buf);
                free(recv_buf);
            }
            else {
                fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
            }
        }
        curl_multi_cleanup(cm);
    }
    curl_multi_cleanup(cm);
    curl_global_cleanup();
}