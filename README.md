#### TRANSPLANT DATA STRUCTURE IN THE LINUX KERNEL

* `struct list_head` and `struct hlist_node`: Change List

    - add the corresponding definition of structure `list_head` and `hlist_node`.
    - add the famous container__of() macro.
    - set LIST__POISON1 and LIST__POISON2 to NULL.
    - remove unnecessary pre-process macro and APIs.
    - consider that the word `new` in C++ is a key word. thus I rename it to newnode.

* `Buffer Ring`: considering one scenario that there is only single consumer and 
single productor , it's possible to implement a lock-free queue.
    - generic buffer ring implementaion
    - lock-free in particular scenario

