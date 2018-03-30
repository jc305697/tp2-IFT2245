Contributor
===========
Frédéric Hamel

Dynamic Array API
=================
- new\_array(int capacity) : Create new instance of array\_t object.
- array\_get\_data(struct array\_t\*) : Get current value of the data array inside the array\_t object.
- array\_get\_size(struct array\_t\*) : Get current number of element inside the array.
- for\_each(struct array\_t\* array, void(\*callback)(void\*)) : Apply the *callback* to each element fo the array.
- delete\_array(struct array\_t \*array) : Delete the array object and call *free* on each element.
  equivalent of *delete_array_callback(array, free)*.
- delete\_array\_callback(struct array\_t array, void(\*deleter)(void\*)) : Delete the array object calling *deleter*
  on every element.

Compile
=======
- Add dyn\_array.c and dyn\_array.h to your sources.
