### PitFalls

- Decrease the page reference count in `kfree()` instead of other places suck as `uvmunmap()`
