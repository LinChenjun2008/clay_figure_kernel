#ifndef __XHCI_RING_H__
#define __XHCI_RING_H__

#define XHCI_COMMAND_RING_TRB_COUNT 256
#define XHCI_EVENT_RING_TRB_COUNT   256
#define XHCI_TRANSFER_RING_COUNT    256

typedef struct xhci_trb_s xhci_trb_t;

typedef struct xhci_ring_s
{
    // TRB ring's virtual address
    xhci_trb_t *trbs;
    // TRB ring's physical address
    phy_addr_t  trbs_paddr;

    size_t      trb_count;
    uint8_t     cycle_bit;

    uint16_t    enqueue;
    uint16_t    dequeue;
} xhci_ring_t;

typedef struct xhci_command_ring_s
{
    xhci_ring_t ring;
} xhci_command_ring_t;

typedef struct xhci_erst_s
{
    xhci_trb_t *rs_addr;
    uint32_t    rs_size;
    uint32_t    rsvdz;
} xhci_erst_t;

typedef struct xhci_event_ring_s
{
    xhci_ring_t  ring;
    xhci_erst_t *erst;
    size_t       erst_count;

} xhci_event_ring_t;

typedef struct xhci_transfer_ring_s
{
    xhci_ring_t ring;
    uint8_t     doorbell_id;
} xhci_transfer_ring_t;

PUBLIC void xhci_trb_enqueue(xhci_ring_t *ring,xhci_trb_t *trb);

#endif