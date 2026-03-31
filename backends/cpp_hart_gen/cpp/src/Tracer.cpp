#include "udb/Tracer.hpp"
#include "udb/iss_soc_model.hpp"
#include "udb/elf_reader.hpp"

namespace udb
{
  Tracer::Tracer() :
    NotificationHandler(nullptr)
  {

  }

  Tracer::~Tracer()
  {

  }

  int Tracer::OnNotification(uint64_t uiEvent, void* pData)
  {
    switch(uiEvent)
    {
    case MEMREAD_EVENT:
      if(pData != nullptr)
      {
        MemAccessRange* pMemAccessRange =  (MemAccessRange*)pData;
        OnPhysicalMemoryRead(pMemAccessRange->GetAddress(), pMemAccessRange->GetSize());
      }
      break;
    case MEMWRITE_EVENT:
      if(pData != nullptr)
      {
        MemAccess* pMemAccess =  (MemAccess*)pData;
        OnPhysicalMemoryWrite(pMemAccess->GetAddress(), pMemAccess->GetSize(), pMemAccess->GetData());
      }
      break;
    default:
      break;
    }
    return 0;
  }

  RiscvTestsTracer::RiscvTestsTracer(HartBase<IssSocModel>* pHart, IssSocModel* pSoC, std::string& elfFilePath) :
    Tracer()
  {
    udb::ElfReader elfReader(elfFilePath.c_str());
    //Is there a "tohost" and/or "fromhost" port (symbol)
    if(elfReader.getSym("tohost", &m_toHostAddress))
      EnableEvent(udb::MEMWRITE_EVENT);
    if(elfReader.getSym("fromhost", &m_fromHostAddress))
      EnableEvent(udb::MEMREAD_EVENT);

    m_pHart = pHart;
    m_pSoC = m_pSoC;
  }

  void RiscvTestsTracer::OnPhysicalMemoryWrite(uint64_t addr, unsigned len, uint64_t data)
  {
    //Capture writes to the "host port"
    if((len == sizeof(uint64_t) && addr == m_toHostAddress) ||
        (len == sizeof(uint32_t) && addr == (m_toHostAddress + sizeof(uint32_t))))
      {
        uint64_t toHostValue = m_pSoC->read_physical_memory_64(m_toHostAddress);

        if((toHostValue & ~(0xffUL)) == 0x0101000000000000UL) //putchar
        {
          DisableNotifications();
          m_pSoC->write_physical_memory_64(m_toHostAddress, 0);
          EnableNotifications();

          putchar((char)(toHostValue & 0xff));
        }
        else if(data < 2)
          throw udb::ExitEvent(0); //Pass
        else
          throw udb::ExitEvent(-1); //fail
      }
  }
}
