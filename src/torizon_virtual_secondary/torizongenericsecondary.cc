/*
 *
 *  TorizonCore Generic Secondary (Prototipe)
 *  Author: SV
 *
 *  Note: This is just an experiment on how to implement a
 *        generic virtual secondary for aktualizr
 *
 */

#include "torizongenericsecondary.h"
#include "uptane/manifest.h"
#include "libaktualizr/types.h"
#include "libaktualizr/config.h"
#include "logging/logging.h"
#include "utilities/fault_injection.h"
#include "utilities/utils.h"
#include "compose_manager.h"
#include "storage/invstorage.h"

#include <typeinfo>
#include <any>

#include <boost/process.hpp>
#include <sstream>
#include <string>
#include <vector>

#define TORIZON_GENERIC_ECU_ID "torizon-generic"
#define DOCKER_COMPOSE_ECU_ID "docker-compose"
#define BOOTLOADER_ECU_ID "bootloader"
#define CORTEXM4_ECU_ID "cortexm4-firmware"

using std::stringstream;

namespace bpo = boost::program_options;

namespace Primary {

const char* const TorizonGenericSecondaryConfig::Type = "torizon-generic";

TorizonGenericSecondaryConfig::TorizonGenericSecondaryConfig(const Json::Value& json_config) : ManagedSecondaryConfig(Type) {
  partial_verifying = json_config["partial_verifying"].asBool();
  ecu_serial = json_config["ecu_serial"].asString();
  ecu_hardware_id = json_config["ecu_hardware_id"].asString();
  full_client_dir = json_config["full_client_dir"].asString();
  ecu_private_key = json_config["ecu_private_key"].asString();
  ecu_public_key = json_config["ecu_public_key"].asString();
  firmware_path = json_config["firmware_path"].asString();
  target_name_path = json_config["target_name_path"].asString();
  metadata_path = json_config["metadata_path"].asString();
  action_script_path = json_config["action_script_path"].asString();
}

std::vector<TorizonGenericSecondaryConfig> TorizonGenericSecondaryConfig::create_from_file(
    const boost::filesystem::path& file_full_path) {
  Json::Value json_config;
  std::ifstream json_file(file_full_path.string());
  Json::parseFromStream(Json::CharReaderBuilder(), json_file, &json_config, nullptr);
  json_file.close();

  std::vector<TorizonGenericSecondaryConfig> sec_configs;
  sec_configs.reserve(json_config[Type].size());

  for (const auto& item : json_config[Type]) {
    sec_configs.emplace_back(TorizonGenericSecondaryConfig(item));
  }
  return sec_configs;
}

void TorizonGenericSecondaryConfig::dump(const boost::filesystem::path& file_full_path) const {
  Json::Value json_config;

  json_config["partial_verifying"] = partial_verifying;
  json_config["ecu_serial"] = ecu_serial;
  json_config["ecu_hardware_id"] = ecu_hardware_id;
  json_config["full_client_dir"] = full_client_dir.string();
  json_config["ecu_private_key"] = ecu_private_key;
  json_config["ecu_public_key"] = ecu_public_key;
  json_config["firmware_path"] = firmware_path.string();
  json_config["target_name_path"] = target_name_path.string();
  json_config["metadata_path"] = metadata_path.string();
  json_config["action_script_path"] = action_script_path.string();
  

  Json::Value root;
  root[Type].append(json_config);

  Json::StreamWriterBuilder json_bwriter;
  json_bwriter["indentation"] = "\t";
  std::unique_ptr<Json::StreamWriter> const json_writer(json_bwriter.newStreamWriter());

  boost::filesystem::create_directories(file_full_path.parent_path());
  std::ofstream json_file(file_full_path.string());
  json_writer->write(root, &json_file);
  json_file.close();
}

TorizonGenericSecondary::TorizonGenericSecondary(Primary::TorizonGenericSecondaryConfig sconfig_in)
    : ManagedSecondary(std::move(sconfig_in)) {
  validateInstall();
}

data::InstallationResult TorizonGenericSecondary::install(const Uptane::Target &target) {
  auto str = secondary_provider_->getTargetFileHandle(target);
   
  printf("--->Saving artifact...\n");
  std::string artifact_file = sconfig.firmware_path.string();
  std::ofstream out_file(artifact_file, std::ios::binary);
  out_file << str.rdbuf();
  str.close();
  out_file.close();
  Utils::writeFile(sconfig.target_name_path, target.filename());
  printf("--->Check Artifact HERE\n");
  printf("--->Install Artifact Here\n");

  /* action_script */
  int ret=std::system((sconfig.action_script_path.string() + " install").c_str());
  if(ret>>8 != 0)
    printf("action script FAIL!\n");

   /* action_script */
  printf("calling action_script validate\n");
  ret=std::system((sconfig.action_script_path.string() + " validate").c_str());
  if(ret>>8 != 0)
    printf("action script FAIL!\n");
  // check here the result from the action script 
  return data::InstallationResult(data::ResultCode::Numeric::kNeedCompletion, "");


//  return data::InstallationResult(data::ResultCode::Numeric::kOk, "OK torizon-generic"); 
}

bool TorizonGenericSecondary::getFirmwareInfo(Uptane::InstalledImageInfo& firmware_info) const {
  std::string content;
 
  
  if (!boost::filesystem::exists(sconfig.target_name_path) || !boost::filesystem::exists(sconfig.firmware_path)) {
    firmware_info.name = std::string("noimage");
    content = "";
  } else {
    firmware_info.name = Utils::readFile(sconfig.target_name_path.string());
    content = Utils::readFile(sconfig.firmware_path.string());
  }
  firmware_info.hash = Uptane::ManifestIssuer::generateVersionHashStr(content);
  firmware_info.len = content.size();
    
  /* action_script */
  int ret=std::system((sconfig.action_script_path.string() + " fw_info").c_str());
  if(ret>>8 != 0)
    printf("action script FAIL!\n");
  
  return true;
}

void TorizonGenericSecondary::validateInstall() {
  
  /* action_script */
  int ret=std::system((sconfig.action_script_path.string() + " validate").c_str());
  printf("RET: %d\n",ret>>8);
}

}  // namespace Primary
