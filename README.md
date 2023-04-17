# introspect

Introspect is a tool I made for the HMN visibility jam that lets you browse a C file as a nonlinear web of declarations and usage sites



https://user-images.githubusercontent.com/28424206/232589489-c7a62b45-c08c-45bc-81e2-6e299bfd74c3.mp4


## Future work:
![image](https://user-images.githubusercontent.com/28424206/232590088-51e4d631-7331-41b2-8b4e-4e9a69ae35a3.png)
This was the design document for the rest of the connections to be made from the parser. What this project really needs is:

1. Work on multiple files via passing the correct arguments to the clang traverse function. In each node show what file it's a part of
2. Allow user to specify include directories/etc to get clang to find includes properly
3. Improve C parse handling code to not ever match via strings like I sometimes do
4. Follow rest of design document to make it more like a DAG instead of having connections be a maximum of 1 layer deep
5. Allow connections between nodes that reach *across files*, color code nodes based on what file they're from
6. Massively speed up everything so point clouds of nodes that are very dense are possible (likely necessary if you show all the nodes traveresed throughout the headers as well)
