## selectops
```
const struct eventop selectops = {
    "select",
    select_init,
    select_add,
    select_del,
    select_dispatch,
    select_dealloc,
    1, /* need_reinit. */
    EV_FEATURE_FDS,
    0,
};
```