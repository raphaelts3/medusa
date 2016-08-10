#include "soci_db.hpp"

#include <medusa/module.hpp>
#include <medusa/log.hpp>
#include <medusa/util.hpp>

SociDatabase::SociDatabase(void)
{
  SociSerializer	toto(m_Session);
  toto.print();
}

SociDatabase::~SociDatabase(void)
{
}

std::string SociDatabase::GetName(void) const
{
  return "SOCI";
}

std::string SociDatabase::GetExtension(void) const
{
  return ".msd";
}

bool SociDatabase::IsCompatible(boost::filesystem::path const& rDatabasePath) const
{
  std::cerr << "---------------------------- DB_Soci: IsCompatible ------------------------" << std::endl;
  return false;
}

bool SociDatabase::Open(boost::filesystem::path const& rDatabasePath)
{
  try
  {
    std::cerr << "---------------------------- DB_Soci: Open ------------------------" << std::endl;
    m_Session.open("sqlite3", "dbname=" + rDatabasePath.string());

    //soci::blob	blob_buffer(m_Session);
    
    // blob_buffer.read(0, static_cast<char*>(m_spBinStrm->GetBuffer()), 0);
    // m_spBinStrm->GetBuffer();
    // m_Session << "INSERT INTO BinaryStream(size, buffer)"
    //   "VALUES(:size, :buffer)",
    //   soci::use(m_spBinStrm->GetSize(), "size"), soci::use(blob_buffer, "buffer");

  }
  catch (std::exception const& e)
  {
    Log::Write("soci").Level(LogError) << e.what() << LogEnd;
    return false;
  }

  return true;
}

bool  SociDatabase::_FileExists(boost::filesystem::path const& rFilePath)
{
  return boost::filesystem::exists(rFilePath);
}

bool  SociDatabase::_FileRemoves(boost::filesystem::path const& rFilePath)
{
  return boost::filesystem::remove(rFilePath);
}

bool SociDatabase::Create(boost::filesystem::path const& rDatabasePath, bool Force)
{
  try
  {
    std::cerr << "------------------------- DB_Soci: Create ----------------------" << std::endl;
    if (Force)
    {
      Log::Write("db_soci") << "remove file " << rDatabasePath.string() << LogEnd;
      _FileRemoves(rDatabasePath);
    }
    if (!Force && _FileExists(rDatabasePath))
    {
      Log::Write("db_soci") << "db already exists and force is false" << LogEnd;
      return false;
    }

    m_Session.open("sqlite3", "dbname=" + rDatabasePath.string());

    m_Session << "CREATE TABLE IF NOT EXISTS BinaryStream("
      "size INTEGER, buffer BLOB)";

    m_Session << "CREATE TABLE IF NOT EXISTS Architecture(architecture_tag INTEGER)";

    m_Session << "CREATE TABLE IF NOT EXISTS MemoryArea("
      "name TEXT, access INTEGER, size INTEGER, architecture_tag INTEGER, "
      "architecture_mode INTEGER, file_offset INTEGER, file_size INTEGER)";

    m_Session << "CREATE TABLE IF NOT EXISTS MappedMemoryArea("
      "name TEXT, access INTEGER, virtual_size INTEGER, architecture_tag INTEGER, "
      "architecture_mode INTEGER, file_offset INTEGER, file_size INTEGER, virtual_address BLOB)";

    m_Session << "CREATE TABLE IF NOT EXISTS Label(type INTEGER,"
      "base INTEGER, offset INTEGER, base_size INTEGER, offset_size INTEGER,"
      "label_addr TEXT, name TEXT, label_type INTEGER, label TEXT, instance_label BLOB)";

    m_Session << "CREATE TABLE IF NOT EXISTS CellData("
      "type INTEGER, sub_type INTEGER, length INTEGER, format_style INTEGER, "
      "architecture_tag INTEGER, architecture_mode INTEGER)";

    m_Session << "CREATE TABLE IF NOT EXISTS MultiCell("
      "dump TEXT, MultiCell BLOB)";

    m_Session << "CREATE TABLE IF NOT EXISTS CrossReference("
      "type_from INTEGER, sub_type_from INTEGER, length_from INTEGER, format_style_from INTEGER, "
      "architecture_tag_from INTEGER, architecture_mode_from INTEGER,"
      "type_to INTEGER, sub_type_to INTEGER, length_to INTEGER, format_style_to INTEGER, "
      "architecture_tag_to INTEGER, architecture_mode_to INTEGER)";
  }
  catch (std::exception const& e)
  {
    Log::Write("soci").Level(LogError) << e.what() << LogEnd;
    exit(0);
    return false;
  }

  return true;
}

bool SociDatabase::Flush(void)
{
  std::cerr << "---------------------------- Flush" << std::endl;
  try
  {
    m_Session.commit();
  }
  catch (std::exception const& e)
  {
    Log::Write("soci").Level(LogError) << e.what() << LogEnd;
    return false;
  }

  return true;
}

bool SociDatabase::Close(void)
{
  try
  {
    std::cerr << "------------------------------- DB_Soci: Close ----------------------------" << std::endl;
    m_Session.close();
  }
  catch (std::exception const& e)
  {
    Log::Write("soci").Level(LogError) << e.what() << LogEnd;
    return false;
  }

  return true;
}

bool SociDatabase::RegisterArchitectureTag(Tag ArchitectureTag)
{
  std::cerr << "---------------------------- RegisterArchitectureTag" << std::endl;
  try
    {
      m_Session << "INSERT INTO Architecture(architecture_tag)"
	"VALUES(:tag)", soci::use(ArchitectureTag);
    }
  catch (std::exception const& e)
    {
      Log::Write("soci").Level(LogError) << e.what() << LogEnd;      
    }
  return true;
}

bool SociDatabase::UnregisterArchitectureTag(Tag ArchitectureTag)
{
  std::cerr << "---------------------------- UnregisterArchitectureTag" << std::endl;
  try
    {
      m_Session << "DELETE FROM Architecture WHERE architecture_tag= :architecture_tag"
	, soci::use(ArchitectureTag);
    }
  catch (std::exception const& e)
    {
      Log::Write("soci").Level(LogError) << e.what() << LogEnd;      
    }
  return true;
}

std::list<Tag> SociDatabase::GetArchitectureTags(void) const
{
  std::cerr << "---------------------------- GetArchitectureTags" << std::endl;
  return{};
}

bool SociDatabase::SetArchitecture(Address const& rAddress, Tag ArchitectureTag, u8 Mode, Database::SetArchitectureModeType SetArchMode)
{
  std::cerr << "----------------------------- DB_Soci: SetArchitecture" << std::endl;
  switch (SetArchMode)
  {
  case ByCell:
  {
    soci::rowset<soci::row> rs = (m_Session.prepare << "SELECT * from MemoryArea");
    for (soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
      {
	soci::row const&	r = *it;
	std::cerr << "<row>";
	for(std::size_t i = 0; i != r.size(); ++i)
	  {
	    const soci::column_properties & props = r.get_properties(i);
		  
	    std::cerr << '<' << props.get_name() << '>';

	    switch(props.get_data_type())
	      {
	      case soci::dt_string:
		std::cerr << r.get<std::string>(i);
		break;
	      case soci::dt_double:
		std::cerr << r.get<double>(i);
		break;
	      case soci::dt_integer:
		std::cerr << r.get<int>(i);
		break;
	      case soci::dt_unsigned_long_long:
		std::cerr << r.get<unsigned long long>(i);
		break;
	      case soci::dt_long_long:
		std::cerr << r.get<long long>(i);
		break;
	      case soci::dt_date:
		std::tm when = r.get<std::tm>(i);
		std::cerr << asctime(&when);
		break;
	      }

	    std::cerr << "</" << props.get_name() << '>' << std::endl;
	  }
	std::cerr << "</row>";
      }
    // for (soci::rowset<MappedMemoryArea>::const_iterator it = RowSet.begin(); it != RowSet.end(); ++it)
    //   {
    // 	std::cerr << "+++++++ FOUND SOMETHING ++++++" << std::endl;
    //   }
    break;
  }

  case ByMemoryArea:
  {
    std::lock_guard<std::mutex>	Lock(m_MemoryAreaLock);
    std::cerr << "------------------------------- BY MEMORY AREA" << std::endl;
    soci::blob	blob(m_Session);
    u32	VirtualSize;
    m_Session << "Select virtual_address, virtual_size FROM MappedMemoryArea"
      , soci::into(blob)
      , soci::into(VirtualSize);

    Address	addr;
    blob.read(0, reinterpret_cast<char*>(&addr), sizeof(Address));
    if ((addr.GetBase() == rAddress.GetBase()) && addr.IsBetween(VirtualSize, rAddress.GetOffset()))
      std::cerr << "----------------------------------- CELL IS PRESENT" << std::endl;
      // m_Session << "UPDATE MemoryArea set name =:test WHERE name=:name"
    //   , soci::use(test, ":test"), soci::use(*pMemArea);
    break;
  }

  default:
    break;
  }

  return false;
}

bool SociDatabase::AddMemoryArea(MemoryArea* pMemArea)
{
  std::cerr << "-------------------------- AddMemoryArea ----------------------"
	    << std::endl
;
  MappedMemoryArea*	pMappedMemArea = dynamic_cast<MappedMemoryArea*>(pMemArea);
  if (pMappedMemArea)
    std::cerr << "------------------------------- DYN_CAST : SUCCESS" << std::endl;
  else
    std::cerr << "------------------------------- DYN_CAST : FAIL" << std::endl;
  std::lock_guard<std::mutex>	Lock(m_MemoryAreaLock);
  std::string	res;
  try
    {
      if (nullptr != pMemArea)
	{
	  soci::blob	blob(m_Session);
	  Address add = pMappedMemArea->GetBaseAddress();
	  blob.write(0, reinterpret_cast<char*>(&add), sizeof(Address));

	  m_Session << "INSERT INTO MappedMemoryArea(name, access, virtual_size, architecture_tag,"
	    "architecture_mode, file_offset, file_size, virtual_address)"
	    "VALUES(:name, :access, :virtual_size, :architecture_tag, "
	    ":architecture_mode, :file_offset, :file_size, :virtual_address)", soci::use (*pMappedMemArea), soci::use(blob, "virtual_address");	  
	  // std::string	name;
	  // m_Session << "SELECT name FROM MemoryArea WHERE access = :access;"
	  //   , soci::into(name), soci::use(*pMemArea);
	  // std::cerr << "----------------------- NAME : " << name << std::endl;
	  
	  // std::string	test("toto");
	  // m_Session << "UPDATE MemoryArea set name =:test WHERE name=:name"
	  //   , soci::use(test, ":test"), soci::use(*pMemArea);

	  // m_Session << "SELECT name FROM MemoryArea WHERE access = :access;"
	  //   , soci::into(name), soci::use(*pMemArea);
	  // std::cerr << "----------------------- NAME : " << name << std::endl;

	}
    }
  catch (soci::soci_error& rErr)
    {
      std::cerr << rErr.what() << std::endl;
      return false;
    }
  
  return true;
}

void SociDatabase::ForEachMemoryArea(MemoryAreaCallback Callback) const
{
  std::cerr << "---------------------------- ForEachMemoryArea" << std::endl;
}

MemoryArea const* SociDatabase::GetMemoryArea(Address const& rAddress) const
{
  std::cerr << "-------------------------- GetMemoryArea ----------------------" << std::endl;

  MappedMemoryArea	*MemArea = new MappedMemoryArea;
  
  soci::blob	blob(m_Session);
  std::string name;
  soci::rowset<soci::row> rowset = (m_Session.prepare << "Select name, access, virtual_size,"
				    "architecture_tag, architecture_mode,"
				    "file_offset, file_size FROM MappedMemoryArea");

  for(soci::rowset<soci::row>::const_iterator it = rowset.begin(); it != rowset.end(); ++it)
    {
      soci::row const& row = *it;
      
      // for (std::size_t i = 0; i < row.size(); ++i)
      // 	{
      // 	  const soci::column_properties & props = row.get_properties(i);
      // 	  std::cerr << " index: " << i << " - name: " << props.get_name() << std::endl;
      // 	}
      std::string Name = row.get<std::string>(0);
      u32 Access = row.get<int>(1);
      u32 VirtualSize = row.get<int>(2);
      u32 ArchitectureTag = row.get<int>(3);
      u8 ArchitectureMode = row.get<int>(4);
      TOffset FileOffset = row.get<int>(5);
      u32 FileSize = row.get<int>(6);

      m_Session << "Select virtual_address FROM MappedMemoryArea WHERE name = :name"
	, soci::use(row.get<std::string>(0)), soci::into(blob);

      Address	addr;
      blob.read(0, reinterpret_cast<char*>(&addr), sizeof(Address));
      MemArea->SetName(Name);
      MemArea->SetAccess(Access);
      MemArea->SetVirtualSize(VirtualSize);
      MemArea->SetDefaultArchitectureTag(ArchitectureTag);
      MemArea->SetDefaultArchitectureMode(ArchitectureMode);
      MemArea->SetFileOffset(FileOffset);
      MemArea->SetFileSize(FileSize);
      MemArea->SetVirtualBase(addr);
      
      if ((addr.GetBase() == rAddress.GetBase()) && addr.IsBetween(VirtualSize, rAddress.GetOffset()))
	{
	  std::cerr << "----------------------------------- GET_MEM_AREA: CELL IS PRESENT" << std::endl;
	  return MemArea;
	}
    }
  return nullptr;
}

bool SociDatabase::GetFirstAddress(Address& rAddress) const
{
  std::cerr << "---------------------------- Get First Address" << std::endl;
  return false;
}

bool SociDatabase::GetLastAddress(Address& rAddress) const
{
  std::cerr << "---------------------------- Get Last Address" << std::endl;
  return false;
}

bool SociDatabase::MoveAddress(Address const& rAddress, Address& rMovedAddress, s64 Offset) const
{
  std::cerr << "---------------------------- Move Address" << std::endl;
  return false;
}
bool SociDatabase::ConvertAddressToPosition(Address const& rAddress, u32& rPosition) const
{
  std::cerr << "---------------------------- Convert Address to Position" << std::endl;
  return false;
}

bool SociDatabase::ConvertPositionToAddress(u32 Position, Address& rAddress) const
{
  std::cerr << "---------------------------- Convert Position to Address" << std::endl;
  return false;
}

bool SociDatabase::AddLabel(Address const& rAddress, Label const& rLabel)
{
  std::cerr << "------------------------------ DB_Soci: AddLabel -------------------------"
	    << std::endl;
  std::lock_guard<std::recursive_mutex>	Lock(m_LabelLock);
  std::cout << "***********************************************************" << std::endl;

  soci::blob	blob(m_Session);
  Label tmpLabel = rLabel;
  blob.write(200, reinterpret_cast<char*>(&tmpLabel), sizeof(Label));
  m_Session << "INSERT INTO Label(type,"
    "base, offset, base_size, offset_size,"
    "label_addr, name, label_type, label, instance_label)"
    "VALUES(:type,"
    ":base, :offset, :base_size, :offset_size,"
    ":label_addr, :name, :label_type, :label, :instance_label)", soci::use(rAddress), soci::use(rLabel), soci::use(blob, "instance_label");

  return true;
}

bool SociDatabase::RemoveLabel(Address const& rAddress)
{
  std::cerr << "----------------------- DB_Soci: RemoveLabel --------------------"
	    << std::endl;
  m_Session << "Delete FROM Label WHERE type = :type AND base = :base AND"
    " offset = :offset AND base_size = :base_size AND offset_size = :offset_size AND"
    " label_addr = :label_addr ", soci::use(rAddress);
  return true;
}

bool SociDatabase::GetLabel(Address const& rAddress, Label& rLabel) const
{
  std::cerr << "----------------------- DB_Soci: GetLabel --------------------"
	    << std::endl;

  std::lock_guard<std::recursive_mutex>	Lock(m_LabelLock);
  soci::blob blob(m_Session);
  m_Session << "SELECT instance_label FROM Label WHERE label_addr = :label_addr"
    , soci::use(rAddress.Dump()), soci::into(blob);
  if (m_Session.got_data())
   blob.read(200, reinterpret_cast<char*>(&rLabel), sizeof(Label));
  else
    {
      std::cerr << "--------------------------- DB_Soci: GetLabel is EMPTY" << std::endl;
      return false;
    }
  return true;
}

bool SociDatabase::GetLabelAddress(Label const& rLabel, Address& rAddress) const
{
  std::cerr << "---------------------------- GetLabelAddress" << std::endl;
  std::lock_guard<std::recursive_mutex> Lock(m_LabelLock);
  soci::indicator ind;
  Address	  address;
  m_Session << "SELECT name FROM Label WHERE name=:name",
    soci::use(rLabel), soci::into(address, ind);
  if (m_Session.got_data())
    {
      switch (ind)
  	{
  	case soci::i_ok:
  	  {
  	    rAddress = address;
  	    std::cerr << "-------------------- DB_Soci, LabelAddress : "
  		      << rAddress.Dump() << std::endl;
  	    return true;
  	  }
  	default:
  	  return false;
  	}
    }
  return false;
}

void SociDatabase::ForEachLabel(LabelCallback Callback)
{
  std::cerr << "---------------------------- ForEachLabel" << std::endl;
}

bool SociDatabase::AddCrossReference(Address const& rTo, Address const& rFrom)
{
  std::lock_guard<std::mutex> Lock(m_CrossReferencesLock);

  std::cerr << "----------------------------------- DB_Soci: AddCrossReference --------------------" << std::endl;
  m_Session << "INSERT INTO CrossReference("
    "type_to INTEGER, sub_type_to INTEGER, length_to INTEGER, format_style_to INTEGER, "
    "architecture_tag_to INTEGER, architecture_mode_to INTEGER)"
    "type_from INTEGER, sub_type_from INTEGER, length_from INTEGER,"
    "format_style_from INTEGER, architecture_tag_from INTEGER, "
    "architecture_mode_from INTEGER,"
    "VALUES("
    ":type_to, sub_type_to, :length_to, :format_style_to, "
    ":architecture_tag_to, :architecture_mode_to)"
    ":type_from, :sub_type_from, :length_from,"
    ":format_style_from, :architecture_tag_from, "
    ":architecture_mode_from)", soci::use(rTo), soci::use(rFrom);

  return true;
}

bool SociDatabase::RemoveCrossReference(Address const& rFrom)
{
  std::cerr << "---------------------------- RemoveCrossReference" << std::endl;
  return false;
}

bool SociDatabase::RemoveCrossReferences(void)
{
  std::cerr << "---------------------------- RemoveCrossReferences" << std::endl;
  return false;
}

bool SociDatabase::HasCrossReferenceFrom(Address const& rTo) const
{
  std::cerr << "---------------------------- HasCrossReferenceFrom" << std::endl;
  return false;
}

bool SociDatabase::GetCrossReferenceFrom(Address const& rTo, Address::List& rFromList) const
{
  std::cerr << "---------------------------- GetCrossReferenceFrom" << std::endl;
  return false;
}

bool SociDatabase::HasCrossReferenceTo(Address const& rFrom) const
{
  std::cerr << "---------------------------- HasCrossReferenceTo" << std::endl;
  return false;
}

bool SociDatabase::GetCrossReferenceTo(Address const& rFrom, Address::List& rToList) const
{
  std::cerr << "---------------------------- GetCrossReferenceTo" << std::endl;
  return false;
}

bool SociDatabase::AddMultiCell(Address const& rAddress, MultiCell const& rMultiCell)
{
  std::cerr << "---------------------------- AddMultiCell" << std::endl;
  soci::blob	blob(m_Session);
  MultiCell MultiCell = rMultiCell;
  blob.write(24, reinterpret_cast<char*>(&MultiCell), sizeof(MultiCell));
  m_Session << "INSERT INTO MultiCell("
    "dump, MultiCell) "
    "VALUES(:dump, :multi_cell)", soci::use(rAddress.Dump()), soci::use(blob);

  return true;
}

bool SociDatabase::RemoveMultiCell(Address const& rAddress)
{
  std::cerr << "---------------------------- RemoveMultiCell" << std::endl;
  return false;
}

bool SociDatabase::GetMultiCell(Address const& rAddress, MultiCell& rMultiCell) const
{
  std::cerr << "---------------------------- GetMultiCell" << std::endl;
  soci::blob	blob(m_Session);
  m_Session << "SELECT multi_cell WHERE name = :name", soci::use(rAddress.Dump())
    , soci::into(blob);
  if (!m_Session.got_data())
    {
      std::cerr << "------------------------ GetMultiCell : EMPTY" << std::endl;
      return false;
    }
  blob.read(24, reinterpret_cast<char*>(&rMultiCell), sizeof(MultiCell));
  return true;
}

bool SociDatabase::GetCellData(Address const& rAddress, CellData& rCellData)
{
  std::cerr << "---------------------------- GetCellData" << std::endl;
  auto pMemArea = GetMemoryArea(rAddress);
  if (pMemArea == nullptr)
    return false;
  auto spCellData = pMemArea->GetCellData(rAddress.GetOffset());
  if (spCellData == nullptr)
    return false;
  rCellData = *spCellData;
  return true;
}

bool SociDatabase::SetCellData(Address const& rAddress, CellData const& rCellData, Address::List& rDeletedCellAddresses, bool Force)
{
  std::cerr << "---------------------------- SetCellData" << std::endl;
  MemoryArea* pCurMemArea = const_cast<MemoryArea*>(GetMemoryArea(rAddress));
  std::cerr << pCurMemArea->Dump() << std::endl;
  if (!pCurMemArea || !pCurMemArea->IsCellPresent(rAddress))
    {
      std::cerr << "---------------------------- SetCellData: CurMemArea is EMPTY or "
		<< "Cell is not present"
		<< std::endl;
      return false;
    }
  std::cerr << " * COUNT *" << std::endl;
  CellData::SPType spCellData = std::make_shared<CellData>(rCellData);
  return pCurMemArea->SetCellData(rAddress.GetOffset(), spCellData, rDeletedCellAddresses,
				  Force);
}

bool SociDatabase::DeleteCellData(Address const& rAddress)
{
  std::cerr << "---------------------------- DeleteCellData" << std::endl;
  return false;
}

bool SociDatabase::GetComment(Address const& rAddress, std::string& rComment) const
{
  std::cerr << "---------------------------- GetComment" << std::endl;
  return false;
}

bool SociDatabase::SetComment(Address const& rAddress, std::string const& rComment)
{
  std::cerr << "---------------------------- SetComment" << std::endl;
  return false;
}

bool SociDatabase::GetValueDetail(Id ConstId, ValueDetail& rConstDtl) const
{
  std::cerr << "---------------------------- GetValueDetail" << std::endl;
  return false;
}

bool SociDatabase::SetValueDetail(Id ConstId, ValueDetail const& rConstDtl)
{
  std::cerr << "---------------------------- SetValueDetail" << std::endl;
  return false;
}

bool SociDatabase::GetFunctionDetail(Id FuncId, FunctionDetail& rFuncDtl) const
{
  std::cerr << "---------------------------- GetFunctionDetail" << std::endl;
  return false;
}

bool SociDatabase::SetFunctionDetail(Id FuncId, FunctionDetail const& rFuncDtl)
{
  std::cerr << "---------------------------- SetFunctionDetail" << std::endl;
  return false;
}

bool SociDatabase::GetStructureDetail(Id StructId, StructureDetail& rStructDtl) const
{
  std::cerr << "---------------------------- GetStructureDetail" << std::endl;
  return false;
}

bool SociDatabase::SetStructureDetail(Id StructId, StructureDetail const& rStructDtl)
{
  std::cerr << "---------------------------- SetStructureDetail" << std::endl;
  return false;
}

bool SociDatabase::RetrieveDetailId(Address const& rAddress, u8 Index, Id& rDtlId) const
{
  std::cerr << "---------------------------- RetrieveDetailId" << std::endl;
  return false;
}

bool SociDatabase::BindDetailId(Address const& rAddress, u8 Index, Id DtlId)
{
  std::cerr << "---------------------------- BindDetailId" << std::endl;
  return false;
}

bool SociDatabase::UnbindDetailId(Address const& rAddress, u8 Index)
{
  std::cerr << "---------------------------- UnbindDetailId" << std::endl;
  return false;
}
