/*******
 * stm32h750 lwIP to ethernet interface
 * device: for model_c
 * date: 21.08.2020
 * documentations: lwIP Wiki
 *******/
 
#include <pbuf.h>
#include <etharp.h>
#include <stm32h750xx.h>
#include "pmc_loader.h"
#include "pins.h"
#include "ethernet.h"

timeout_t const netif_BLOCK_TIMEOUT = RTOS_MS(250);

TEthernetProc EthernetProc("Ethernet");

uint8_t mac_address[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

static dma_tx_descriptor tx_desc[8] __attribute__((aligned(4), __section__(".eth")));
static dma_rx_descriptor rx_desc[8] __attribute__((aligned(4), __section__(".eth")));
static uint8_t eth_tx_buffer[8][1532] __attribute__((aligned(4), __section__(".eth")));
static uint8_t eth_rx_buffer[8][1532] __attribute__((aligned(4), __section__(".eth")));
static uint8_t eth_tx_index, eth_rx_index;
uint8_t eth_load_buffer[1532];
uint16_t eth_load_len[8];

uint8_t eth_fd_ld_err, eth_es_err;

bool phy_auto_neg     = 0,
     phy_auto_neg_pre = 0,
     phy_link         = 0,
     phy_link_pre     = 0;

OS::TEventFlag eth_irq_request;
static OS::TMutex tx_mutex;
netif NetIF;

void lan8740_init();

//---------------------------------------------------------------------------

static err_t ethernetif_output(struct netif *, struct pbuf *p)
{
    struct pbuf *q;
    int l = 0;
    uint8_t *buffer = (uint8_t *)&eth_tx_buffer[eth_tx_index];
    if(tx_mutex.try_lock(netif_BLOCK_TIMEOUT))
    {
        for(q = p; q != NULL; q = q->next)
        {
            memcpy((uint8_t *)&buffer[l], q->payload, q->len);
            l = l + q->len;
        }

        tx_desc[eth_tx_index].tdes0 = (uint32_t)&eth_tx_buffer[eth_tx_index];
        tx_desc[eth_tx_index].tdes2 = ETH_TDES2_IOC | l;
        tx_desc[eth_tx_index].tdes3 = ETH_TDES3_OWN | ETH_TDES3_FD | ETH_TDES3_LD | (l & 0x00007FFF);
        __DSB();
        // clear TBU flag to resume processing
        ETH->DMACSR = ETH_DMACSR_TBU;
        // instruct the DMA to poll the transmit descriptor list
        ETH->DMACTDTPR = 0;
        if(++eth_tx_index >= 8)
        {
            eth_tx_index = 0;
        }
        while((tx_desc[eth_tx_index].tdes3 & ETH_TDES3_OWN) != 0)
        {}
        print("packet transmitted\r\n");
        tx_mutex.unlock();
    }
    return ERR_OK;
}
//---------------------------------------------------------------------------

void ethernetif_input()
{   
    struct pbuf *p = NULL;
    struct pbuf *q = NULL;
    uint16_t len = 0;
    uint8_t *buffer;

    if(netif_is_link_up(&NetIF) != phy_link)
    {
        if(phy_link)
            netif_set_link_up(&NetIF);
        else
            netif_set_link_down(&NetIF);
    }
    
    // buffer is available
    if(!(rx_desc[eth_rx_index].rdes3 & ETH_RDES3_OWN))
    {
        // first and last descriptor
        if((rx_desc[eth_rx_index].rdes3 & ETH_RDES3_FD) && (rx_desc[eth_rx_index].rdes3 & ETH_RDES3_LD))
        {
            // no error
            if(!(rx_desc[eth_rx_index].rdes3 & ETH_RDES3_ES))
            {
                buffer = (uint8_t *)&eth_rx_buffer[eth_rx_index];
                // frame length
                len = (rx_desc[eth_rx_index].rdes3 & ETH_RDES3_PL) - 4;
                
                if(len > 0)
                {
                    p = pbuf_alloc(PBUF_RAW, len,PBUF_POOL);
                }
                if(p != NULL)
                {
                    for(q = p; q != NULL; q = q->next)
                    {
                        memcpy((uint8_t *)q->payload, (uint8_t *)buffer, q->len);
                    }
                }
                err_t err = NetIF.input(p, &NetIF);
                if (err != ERR_OK)
                {
                    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
                    pbuf_free(p);
                }
            }
            else eth_es_err++;
        }
        else eth_fd_ld_err++;
        rx_desc[eth_rx_index].rdes0 = (uint32_t)(&eth_rx_buffer[eth_rx_index]);
        rx_desc[eth_rx_index].rdes3 = ETH_RDES3_OWN | ETH_RDES3_IOC | ETH_RDES3_BUF1V;
        if(++eth_rx_index >= 8)
        {
            eth_rx_index = 0;
        }
        print("packet received\r\n");
    }
    // clear RBU flag to resume processing, clear RI and NIS
    ETH->DMACSR = ETH_DMACSR_RBU | ETH_DMACSR_RI | ETH_DMACSR_NIS;
    // instruct the DMA to poll the receive descriptor list
    ETH->DMACRDTPR = 0;
    
    // enable DMA interrupts
    ETH->DMACIER = ETH_DMACIER_NIE | ETH_DMACIER_RIE;
}

#include    <lwip/timeouts.h>
void arp_timer(void *)
{
    etharp_tmr();
    sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
}

//---------------------------------------------------------------------------
//
//                               lwIP init
// 
//  
err_t ethernetif_init(struct netif *netif)
{
    uint8_t index;
    
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    /*
    * Initialize the snmp variables and counters inside the struct netif.
    * The last argument should be replaced with your link speed, in units
    * of bits per second.
    */
//    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100000000);
 
    /* We directly use etharp_output() here to save a function call.
    * You can instead declare your own function an call etharp_output()
    * from it if you have to do some checks before sending (e.g. if link
    * is available...) */
    netif->output = etharp_output;
    netif->linkoutput = ethernetif_output;
                                                
    /* initialize the hardware */
    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    /* set MAC hardware address */
    netif->hwaddr[0] =  mac_address[0];
    netif->hwaddr[1] =  mac_address[1];
    netif->hwaddr[2] =  mac_address[2];
    netif->hwaddr[3] =  mac_address[3];
    netif->hwaddr[4] =  mac_address[4];
    netif->hwaddr[5] =  mac_address[5];
    /* maximum transfer unit */
    netif->mtu = 1500;
    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOHEN;
    
    NVIC_EnableIRQ(ETH_IRQn);

    //                                      _______*** pins ***_______
    // 
    //                          _______* MII_RX_CLK *_______

    MII_RX_CLK::Mode(ALT_OUTPUT);
    MII_RX_CLK::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_RXD0 *_______
    MII_RXD0::Mode(ALT_OUTPUT);
    MII_RXD0::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_RXD1 *_______
    MII_RXD1::Mode(ALT_OUTPUT);
    MII_RXD1::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_RXD2 *_______
    MII_RXD2::Mode(ALT_OUTPUT);
    MII_RXD2::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_RXD3 *_______
    MII_RXD3::Mode(ALT_OUTPUT);
    MII_RXD3::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_CRS *_______
    MII_CRS::Mode(ALT_OUTPUT);
    MII_CRS::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_COL *_______
    MII_COL::Mode(ALT_OUTPUT);
    MII_COL::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_TX_CLK *_______
    MII_TX_CLK::Mode(ALT_OUTPUT);
    MII_TX_CLK::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_TX_EN *_______
    MII_TX_EN::Mode(ALT_OUTPUT);
    MII_TX_EN::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_TXD0 *_______
    MII_TXD0::Mode(ALT_OUTPUT);
    MII_TXD0::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_TXD1 *_______
    MII_TXD1::Mode(ALT_OUTPUT);
    MII_TXD1::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_TXD2 *_______
    MII_TXD2::Mode(ALT_OUTPUT);
    MII_TXD2::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_TXD3 *_______
    MII_TXD3::Mode(ALT_OUTPUT);
    MII_TXD3::Alternate(ALT_FUNC_ETH);
    
    //                          _______* MII_RX_ER *_______
    MII_RX_ER::Mode(ALT_OUTPUT);
    MII_RX_ER::Alternate(ALT_FUNC_ETH);
    
    //                          _______* MII_RX_DV *_______
    MII_RX_DV::Mode(ALT_OUTPUT);
    MII_RX_DV::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_MDC *_______
    MII_MDC::Mode(ALT_OUTPUT);
    MII_MDC::Alternate(ALT_FUNC_ETH);

    //                          _______* MII_MDIO *_______
    MII_MDIO::Mode(ALT_OUTPUT);
    MII_MDIO::Alternate(ALT_FUNC_ETH);
    
    //                          _______* INT *_______
    INT::Mode(INPUT);
    
    //                                      _______* SYSCFG clock enable *_______
    //RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    
    //                                      _______* set RMII *_______
    //SYSCFG->PMCR |= SYSCFG_PMCR_EPIS_SEL_2;
    
    //                                      _______*** clocks ***_______
    
    RCC->AHB1ENR |= RCC_AHB1ENR_ETH1MACEN | RCC_AHB1ENR_ETH1RXEN | RCC_AHB1ENR_ETH1TXEN;
    
    //                                      _______*** ethernet reset ***_______
    
    RCC->AHB1RSTR |= RCC_AHB1RSTR_ETH1MACRST;
    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_ETH1MACRST;
    
    //                                      _______*** software reset ***_______
    ETH->DMAMR |= ETH_DMAMR_SWR;
    // Wait for the reset to complete
    while(ETH->DMAMR & ETH_DMAMR_SWR)
    {
    }
    
    //                                      _______*** MDIO clock ***_______
    ETH->MACMDIOAR = ETH_MACMDIOAR_CR_DIV124;
    
    //                                      _______* PHY init *_______
    lan8740_init();
    
    //                                      _______*** receive if tx_en ***_______    
    //ETH->MACCR |= ETH_MACCR_DO;
    //                                      _______*** 100 Mbps ***_______    
    ETH->MACCR |= ETH_MACCR_FES;
    //                                      _______*** duplex mode ***_______    
    ETH->MACCR |= ETH_MACCR_DM;
    //                                      _______*** TX RX enable ***_______    
    ETH->MACCR |= ETH_MACCR_TE | ETH_MACCR_RE;
    
    //                                      _______*** MAC address ***_______    
    ETH->MACA0HR   = ((uint32_t)mac_address[5] << 8) |
                     ((uint32_t)mac_address[4] << 0);
    ETH->MACA0LR   = ((uint32_t)mac_address[3] << 24) |
                     ((uint32_t)mac_address[2] << 16) |
                     ((uint32_t)mac_address[1] << 8) |
                     ((uint32_t)mac_address[0] << 0);
    
    //                                      _______* enable flow control *_______
    ETH->MACTFCR |= ETH_MACTFCR_TFE;
    ETH->MACRFCR |= ETH_MACRFCR_RFE;
    
    //                                      _______* filtering control *_______
    //ETH->MACPFR |= ETH_MACPFR_RA;
    
    //                                      _______* address-aligned beats *_______
    ETH->DMASBMR |= ETH_DMASBMR_AAL;
    
    //                                      _______* start or stop transmit *_______
    ETH->DMACTCR |= ETH_DMACTCR_ST;
    //                                      _______* start or stop receive *_______
    ETH->DMACRCR |= ETH_DMACRCR_SR;
    
    //                                      _______*** descriptors ***_______
    
    
    for(index = 0; index < 8; index++)
    {
        tx_desc[index].tdes0 = 0;
        tx_desc[index].tdes1 = 0;
        tx_desc[index].tdes2 = 0;
        tx_desc[index].tdes3 = 0;
    }
    eth_tx_index = 0;
    
    for(index = 0; index < 8; index++)
    {
        rx_desc[index].rdes0 = (uint32_t)(&eth_rx_buffer[index]);
        rx_desc[index].rdes1 = 0;
        rx_desc[index].rdes2 = 0;
        rx_desc[index].rdes3 = ETH_RDES3_OWN | ETH_RDES3_IOC | ETH_RDES3_BUF1V;
    }
    eth_rx_index = 0;
    
    // start location of the TX descriptor
    ETH->DMACTDLAR = (uint32_t)&tx_desc[0];
    // length of the transmit descriptor ring
    ETH->DMACTDRLR = 7;
    
    // start location of the RX descriptor
    ETH->DMACRDLAR = (uint32_t)&rx_desc[0];
    // length of the receive descriptor ring
    ETH->DMACRDRLR = 7;
    
    // disable MAC interrupts
    ETH->MACIER = 0;
    
    etharp_init();
    sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
    
    // enable DMA interrupts
    ETH->DMACIER = ETH_DMACIER_NIE | ETH_DMACIER_RIE;

    return ERR_OK;
    
}

//---------------------------------------------------------------------------
//
//                               Phy command and init
// 
//
uint16_t eth_phy_read(uint8_t phy_address, uint8_t reg_address)
{
    uint32_t temp;
    uint16_t data;
    
    // MDIO clock configuration to temp
    temp = ETH->MACMDIOAR & ETH_MACMDIOAR_CR;
    // read code & MII busy to temp
    temp |= ETH_MACMDIOAR_MOC_RD | ETH_MACMDIOAR_MB;
    // phy address to temp
    temp |= (phy_address << 21) & ETH_MACMDIOAR_PA;
    // register address to temp
    temp |= (reg_address << 16) & ETH_MACMDIOAR_RDA;
    
    // start read
    ETH->MACMDIOAR = temp;
    // wait for the read to complete
    while(ETH->MACMDIOAR & ETH_MACMDIOAR_MB) {}
    // get register value
    data = ETH->MACMDIODR & ETH_MACMDIODR_MD;
    
    return data;
    
}

//---------------------------------------------------------------------------

void eth_phy_write(uint8_t phy_address, uint8_t reg_address, uint16_t data)
{
    uint32_t temp;
    
    // MDIO clock configuration to temp
    temp = ETH->MACMDIOAR & ETH_MACMDIOAR_CR;
    // read code & MII busy to temp
    temp |= ETH_MACMDIOAR_MOC_RD | ETH_MACMDIOAR_MB;
    // phy address to temp
    temp |= (phy_address << 21) & ETH_MACMDIOAR_PA;
    // register address to temp
    temp |= (reg_address << 16) & ETH_MACMDIOAR_RDA;
    
    // to be written data
    ETH->MACMDIODR = data & ETH_MACMDIODR_MD;
    
    // start write
    ETH->MACMDIOAR = temp;
    // wait for the write to complete
    while(ETH->MACMDIOAR & ETH_MACMDIOAR_MB) {}
    
}

//---------------------------------------------------------------------------

void lan8740_init()
{    
    //reset
    eth_phy_write(0x01, LAN8740_BCR, BCR_RESET);
    //wait until reset done
    while(eth_phy_read(0x01, LAN8740_BCR) & BCR_RESET) {}
    
    // set advertisement
    eth_phy_write(0x01, LAN8740_ANAR, 0
                  | ANAR_100BTX
                  | ANAR_100BTX_FD
                  | ANAR_PAUSE0
                  | ANAR_PAUSE1
                 );
    
    // enable auto negotiation
    eth_phy_write(0x01, LAN8740_BCR, 0
                  | BCR_AN_EN
                  | BCR_RESTART_AN
                 );
    
    // enable interrupt
    eth_phy_write(0x01, LAN8740_IMR, 0
                  | IMR_AN_COMPLETE
                  | IMR_LINK_DOWN
                 );
}

//---------------------------------------------------------------------------

void lan8740_status()
{
    uint16_t status_reg;
    
    // read LAN8740 BSR
    status_reg = eth_phy_read(0x01, LAN8740_BSR);
    
    // auto-neg status
    phy_auto_neg = status_reg & BSR_AN_COMPLETE;
    if(phy_auto_neg != phy_auto_neg_pre)
    {
        if(phy_auto_neg)
        {
            print("auto-neg complete\r\n");
        }
        else 
        {
            print("auto-neg not complete\r\n");
        }
    }
    phy_auto_neg_pre = phy_auto_neg;
    
    // link status
    phy_link = status_reg & BSR_LINK_STATUS;
    if(phy_link != phy_link_pre)
    {
        if(phy_link)
        {
            print("link is up\r\n");
        }
        else 
        {
            print("link is down\r\n");
        }
        
        //OS::TISRW ISR;
    
        //eth_irq_request.signal_isr();
    }
    phy_link_pre = phy_link;
}

//---------------------------------------------------------------------------
//
//                               Ethernet command
// 
//
void eth_tx_buffer_init()
{
    uint8_t i;
    uint16_t cnt;
    
    for(i = 0; i < 8; i++)
    {
        for(cnt = 0; cnt < 1532; cnt++)
        {
            if(cnt < 256) eth_tx_buffer[i][cnt] = i;
            else eth_tx_buffer[i][cnt] = 0;
        }
    }
}

//---------------------------------------------------------------------------


extern "C" void ETH_IRQHandler(void)
{
    // disable normal interrupt
    ETH->DMACIER &= ~ETH_DMACIER_NIE;

    OS::TISRW ISR;
    
    eth_irq_request.signal_isr();
    
}

//---------------------------------------------------------------------------
 
namespace OS
{

template<> void TEthernetProc::exec()
{
    for(;;)
    {
        
        eth_irq_request.wait();
        print("eth_irq\r\n");
        ethernetif_input();
    }
}
}



