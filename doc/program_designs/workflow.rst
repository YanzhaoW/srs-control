########################
 Data Analysis Workflow
########################

*************************
 Buffer and buffer queue
*************************

Buffer queue is essential for the program to read the data from UDP sockets and consume data asynchronously. The buffer queue has the type :cpp:class:`srs::BufferQueue`, which contains a vector of buffer elements of the type :cpp:class:`srs::LargeBuffer`. The simple way of managing the insert and poping of the buffer element would be copying the buffer element, stored in the :cpp:class:`srs::connection::DataSocket`, into the buffer queue, and later pop any available buffer inside the buffer queue to the buffer elements stored in :cpp:class:`srs::workflow::TaskDiagram` for the data analysis taskflow. However, this process would be really slow due to the copying the constantly memory allocation of the read buffer in the data socket. Instead of copying the whole buffer into the buffer queue, a much more efficient way is to create an empty buffer first in the buffer queue and immediately swap it with the buffer from the data socket. But again, the data socket buffer is empty and needs memory allocation again for the UDP reading. This can be solved by using another buffer queue to store the all the used buffer from the :cpp:class:`srs::workflow::TaskDiagram` and swap it with the data socket buffer. Therefore, throughout the whole reading and analysis process, no memory allocation occurs.

.. figure:: /media/buffer_queue.svg
    :align: center

    Figure 2: The push and pop of the buffer queue.

The interactions between the buffer queue and the UDP sockets is completely asynchronous with the interactions between the buffer queue and analysis taskflows. Regarding to the former one, it can be described chronically as the following:

1. UDP socket receives data from the remote server, which is stored in its buffer of the type ``LargeBuffer`` along with a data size of the received message.
2. According to the given size, the ``resize()`` member function is called, which just changes size of the buffer, but without changing any values outside of its size.
3. The (non-const) reference of the socket buffer is given to the buffer queue.
4. The socket buffer is pushed to the **valid buffer queue** by first creating an empty buffer with both the size and capacity to be 0 in place of the tail of the queue and swapping the new empty buffer with the socket buffer, which contains the valid message. The swapping action is just the exchange of the internal data addresses of the two buffer.
5. If the valid buffer queue returns ``false`` from the call of ``try_emplace``, it means the valid buffer queue is full and data loss will be reported. In such case, the swapping action fails and the socket buffer is non-empty and given directly back the to socket.
6. If the valid buffer queue is not full. The socket buffer becomes empty and filled with a buffer from the **trash buffer queue**. Similar to the pushing process, the filling process swaps the non-empty head buffer of the queue with the empty socket buffer. The non-empty socket buffer is then given back to the socket.
7. Goes back to step 1 when new data is received from the UDP socket.

On the other hand, interaction between the buffer queue and the analysis taskflow can be described as following:

1. The input buffer of the taskflow is initialized when the taskflow is constructed. The initialization allocates the input buffer with maximal number (``buffer_size``) of zero values.
2. Once the taskflow starts, it pushes the input buffer to the tail of the trash buffer queue.
3. After the pushing, the input buffer becomes empty and then filled with the head buffer of the valid buffer queue.
4. The non-empty input buffer is then given to the analysis taskflow.
5. Goes back to step 1 once the current turn of the analysis taskflow is finished.
