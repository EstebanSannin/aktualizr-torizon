#ifndef PRIMARY_TORIZONGENERICSECONDARY_H_
#define PRIMARY_TORIZONGENERICSECONDARY_H_

#include <string>
#include "managedsecondary.h"
#include "libaktualizr/types.h"

namespace Primary {

class TorizonGenericSecondaryConfig : public ManagedSecondaryConfig {
 public:
  TorizonGenericSecondaryConfig() : ManagedSecondaryConfig(Type) {}
  TorizonGenericSecondaryConfig(const Json::Value& json_config);

  static std::vector<TorizonGenericSecondaryConfig> create_from_file(const boost::filesystem::path& file_full_path);
  void dump(const boost::filesystem::path& file_full_path) const;

 public:
  static const char* const Type;
};

/**
 * An primary secondary that runs on the same device but treats
 * the firmware that it is pushed as a docker-compose yaml file
 */
class TorizonGenericSecondary : public ManagedSecondary {
 public:
  explicit TorizonGenericSecondary(Primary::TorizonGenericSecondaryConfig sconfig_in);
  ~TorizonGenericSecondary() override = default;

  std::string Type() const override { return TorizonGenericSecondaryConfig::Type; }

  bool ping() const override { return true; }

 private:
  bool getFirmwareInfo(Uptane::InstalledImageInfo& firmware_info) const override;
  data::InstallationResult install(const Uptane::Target &target) override;
  void validateInstall();
};

}  // namespace Primary

#endif  // PRIMARY_TORIZONGENERICSECONDARY_H_
