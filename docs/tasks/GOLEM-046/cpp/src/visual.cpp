// A second generated package lives in its own translation unit.
#include "generated/visual.h"
extern "C" int review_visual(){
    int result=golem_agnb_stock_load();if(result)return result;
    golem_agnb_stock_setup();golem_stock_drawPackage();golem_agnb_stock_cleanup();
    return 0;
}
