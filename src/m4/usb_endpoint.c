#include "usb_endpoint.h"
#include "usb_queue.h"

USBEndpoint* usb_endpoint_create(USBEndpoint *endpoint, uint8_t bEndpointAddress, 
    USBDevice *device, USBEndpoint *other_endpoint, 
    Endpoint_cb setup_complete, Endpoint_cb transfer_complete
    ) 
{
    endpoint->address = bEndpointAddress;
    endpoint->device = device;
   
    endpoint->setup_complete = setup_complete;
    endpoint->transfer_complete = transfer_complete;

    // if IN endpoint
    if (endpoint->address & 0x80) {
        endpoint->in =  endpoint;
        endpoint->out = other_endpoint;
    } else {
        endpoint->in = other_endpoint;
        endpoint->out = endpoint;
    }

    return endpoint;
}
bool usb_endpoint_alloc_queue(USBEndpoint *endpoint, size_t pool_size, void*(*alloc_cb)(size_t))    
{
    USBTransfer *transfers = alloc_cb(sizeof(USBTransfer) * pool_size);
    if (!transfers) {
        return false;
    }
    USBQueue *queue = alloc_cb(sizeof(USBQueue));
    if (!queue) {
        return false;
    }
    queue->endpoint = endpoint;
    queue->free_transfers = transfers;
    queue->pool_size = pool_size;
    usb_queue_init(queue);
    return true;
}