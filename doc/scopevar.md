# scope

variable scope

``` c
xen = {
    l = [5,3]
    p = "nig"
}
// xen.l : list
// xen.p : str
```

named scope

variable has serveral named scope

``` c
prog("xxx") {
    l = [5,3]
    p = "nig"
}
// prog.xxx.l : list
// prog.xxx.p : str
```