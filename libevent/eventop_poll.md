## pollops
```
const struct eventop pollops = {
    "poll",
    poll_init,
    poll_add,
    poll_del,
    poll_dispatch,
    poll_dealloc,
    1, /* need_reinit */
    EV_FEATURE_FDS|EARLY_CLOSE_IF_HAVE_RDHUP,
    sizeof(struct pollidx),
};
```