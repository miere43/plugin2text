#include "tes.hpp"
#include "common.hpp"
#include "typeinfo.hpp"
#include "array.hpp"
#include <string.h>
#include <stdlib.h>

bool RawRecord::is_compressed() const {
    return is_bit_set(flags, RecordFlags::Compressed);
}

const char* record_group_type_to_string(RecordGroupType type) {
    switch (type) {
        case RecordGroupType::Top:                    return "Top";
        case RecordGroupType::WorldChildren:          return "World";
        case RecordGroupType::InteriorCellBlock:      return "Interior Block";
        case RecordGroupType::InteriorCellSubBlock:   return "Interior Sub-Block";
        case RecordGroupType::ExteriorCellBlock:      return "Exterior";
        case RecordGroupType::ExteriorCellSubBlock:   return "Exterior Sub-Block";
        case RecordGroupType::CellChildren:           return "Cell";
        case RecordGroupType::TopicChildren:          return "Topic";
        case RecordGroupType::CellPersistentChildren: return "Persistent";
        case RecordGroupType::CellTemporaryChildren:  return "Temporary";
        default: verify(false); return nullptr;
    }
}

static const char* const ShortMonthNames[12]{
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec",
};

const char* month_to_short_string(int month) {
    verify(month >= 1 && month <= 12);
    return ShortMonthNames[month - 1];
}

int short_string_to_month(const char* str) {
    auto count = strlen(str);
    verify(count == 3);
    for (int i = 0; i < _countof(ShortMonthNames); ++i) {
        if (memory_equals(ShortMonthNames[i], str, 3)) {
            return i + 1;
        }
    }
    verify(false);
    return 0; 
}

Array<VMAD_Script> VMAD_Field::parse_scripts(BinaryReader& r, uint16_t script_count, bool preserve_property_order) {
    Array<VMAD_Script> scripts{ tmpalloc };
    for (int i = 0; i < script_count; ++i) {
        VMAD_Script script;
        script.parse(r, this, preserve_property_order);
        scripts.push(script);
    }
    return scripts;
}

VMAD_Field::VMAD_Field() {
    memset(this, 0, sizeof(this));
    scripts = Array<VMAD_Script>(tmpalloc);
}

void VMAD_Field::parse(const uint8_t* value, size_t size, RecordType record_type, bool preserve_property_order) {
    BinaryReader r;
    r.start = value;
    r.now = r.start;
    r.end = r.start + size;

    const auto header = r.advance<VMAD_Header>();
    verify(header->version >= 2 && header->version <= 5);
    verify(header->object_format >= 1 && header->object_format <= 2);

    this->version = header->version;
    this->object_format = header->object_format;
    this->scripts = parse_scripts(r, header->script_count, preserve_property_order);
    this->contains_record_specific_info = r.now != r.end;

    if (contains_record_specific_info) {
        switch (record_type) {
            case RecordType::INFO: {
                verify(r.read<uint8_t>() == 2); // version?

                info.flags = r.read<PapyrusFragmentFlags>();
                info.script_name = r.advance_wstring();

                if (is_bit_set(info.flags, PapyrusFragmentFlags::HasBeginScript)) {
                    info.start_fragment.parse(r);
                }
                if (is_bit_set(info.flags, PapyrusFragmentFlags::HasEndScript)) {
                    info.end_fragment.parse(r);
                }
            } break;

            case RecordType::QUST: {
                verify(r.read<uint8_t>() == 2); // version?

                auto fragment_count = (int)r.read<uint16_t>();
                qust.file_name = r.advance_wstring();

                qust.fragments = Array<VMAD_QUST_Fragment>{ tmpalloc };
                for (int frag_index = 0; frag_index < fragment_count; ++frag_index) {
                    VMAD_QUST_Fragment fragment;
                    fragment.parse(r);
                    qust.fragments.push(fragment);
                };

                auto alias_count = (int)r.read<uint16_t>();
                qust.aliases = Array<VMAD_QUST_Alias>{ tmpalloc }; // @TODO: Use StaticArray
                for (int alias_index = 0; alias_index < alias_count; ++alias_index) {
                    VMAD_QUST_Alias alias;
                    
                    VMAD_ScriptPropertyValue value;
                    value.parse(r, this, PapyrusPropertyType::Object);
                    alias.object = value.as_object;
                    
                    verify(r.read<uint16_t>() == header->version);
                    verify(r.read<uint16_t>() == header->object_format);

                    const auto script_count = r.read<uint16_t>();
                    alias.scripts = parse_scripts(r, script_count, preserve_property_order);
                }
            } break;

            case RecordType::PACK: {
                // @TODO @Test
                verify(r.read<uint8_t>() == 2); // version?

                pack.flags = r.read<VMAD_PACK_Flags>();
                pack.file_name = r.advance_wstring();

                if (is_bit_set(pack.flags, VMAD_PACK_Flags::OnBegin)) {
                    pack.begin_fragment.parse(r);
                }
                if (is_bit_set(pack.flags, VMAD_PACK_Flags::OnEnd)) {
                    pack.end_fragment.parse(r);
                }
                if (is_bit_set(pack.flags, VMAD_PACK_Flags::OnChange)) {
                    pack.change_fragment.parse(r);
                }
            } break;

            case RecordType::PERK: {
                // @TODO @Test
                verify(r.read<uint8_t>() == 2); // version?

                perk.file_name = r.advance_wstring();
                const auto fragment_count = r.read<uint16_t>();

                perk.fragments = Array<VMAD_PERK_Fragment>{ tmpalloc }; // @TODO: Use StaticArray
                for (int i = 0; i < fragment_count; ++i) {
                    VMAD_PERK_Fragment fragment;
                    fragment.parse(r);
                    perk.fragments.push(fragment);
                }
            } break;

            case RecordType::SCEN: {
                // @TODO @Test
                verify(r.read<uint8_t>() == 2); // version?

                scen.flags = r.read<PapyrusFragmentFlags>();
                scen.file_name = r.advance_wstring();

                if (is_bit_set(info.flags, PapyrusFragmentFlags::HasBeginScript)) {
                    scen.begin_fragment.parse(r);
                }
                if (is_bit_set(info.flags, PapyrusFragmentFlags::HasEndScript)) {
                    scen.end_fragment.parse(r);
                }

                const auto phase_count = r.read<uint16_t>();
                scen.phase_fragments = Array<VMAD_SCEN_PhaseFragment>{ tmpalloc };
                for (int i = 0; i < phase_count; ++i) {
                    VMAD_SCEN_PhaseFragment fragment;
                    fragment.parse(r);
                    scen.phase_fragments.push(fragment);
                }
            } break;
        }
    }

    verify(r.now == r.end);
}

void VMAD_Script::parse(BinaryReader& r, const VMAD_Field* vmad, bool preserve_property_order) {
    name = r.advance_wstring();
    if (vmad->version >= 4) {
        status = r.read<uint8_t>();
    }

    auto property_count = r.read<uint16_t>();
    for (int prop_index = 0; prop_index < property_count; ++prop_index) {
        VMAD_ScriptProperty property;
        property.parse(r, vmad);
        properties.push(property);
    }

    if (!preserve_property_order) {
        qsort(properties.data, properties.count, sizeof(properties.data[0]), [](void const* aa, void const* bb) -> int {
            auto a = ((VMAD_ScriptProperty*)aa)->name;
            auto b = ((VMAD_ScriptProperty*)bb)->name;
            auto min_count = a->count > b->count ? b->count : a->count;
            int cmp = strncmp(a->data, b->data, min_count);
            return cmp == 0 ? a->count - b->count : cmp;
        });
    }
}

void VMAD_ScriptProperty::parse(BinaryReader& r, const VMAD_Field* vmad) {
    name = r.advance_wstring();
    type = r.read<PapyrusPropertyType>();
    if (vmad->version >= 4) {
        status = r.read<uint8_t>();
    }
    value.parse(r, vmad, type);
}

VMAD_ScriptPropertyValue::VMAD_ScriptPropertyValue() {
    memset(this, 0, sizeof(*this));
}

void VMAD_ScriptPropertyValue::parse(BinaryReader& r, const VMAD_Field* vmad, PapyrusPropertyType type) {
    verify(vmad->object_format == 2);
    switch (type) {
        case PapyrusPropertyType::Object: {
            auto value = r.read<VMAD_PropertyObjectV2>();
            as_object.form_id = value.form_id;
            as_object.alias = value.alias;
            verify(value.unused == 0);
        } break;

        case PapyrusPropertyType::String: {
            as_string = r.advance_wstring();
        } break;

        case PapyrusPropertyType::Int: {
            as_int = r.read<int>();
        } break;

        case PapyrusPropertyType::Float: {
            as_float = r.read<float>();
        } break;

        case PapyrusPropertyType::Bool: {
            as_bool = r.read<bool>();
        } break;

        case PapyrusPropertyType::ObjectArray:
        case PapyrusPropertyType::StringArray:
        case PapyrusPropertyType::IntArray:
        case PapyrusPropertyType::FloatArray:
        case PapyrusPropertyType::BoolArray: {
            const auto inner_type = (PapyrusPropertyType)((uint32_t)type - 10);
            as_array.count = r.read<uint32_t>();
            as_array.values = new VMAD_ScriptPropertyValue[as_array.count];
            for (uint32_t i = 0; i < as_array.count; ++i) {
                as_array.values[i].parse(r, vmad, inner_type);
            }
        } break;

        default: {
            verify(false);
        } break;
    }
}

void VMAD_INFO_Fragment::parse(BinaryReader& r) {
    verify(1 == r.read<uint8_t>());
    script_name = r.advance_wstring();
    fragment_name = r.advance_wstring();
}

void VMAD_QUST_Fragment::parse(BinaryReader& r) {
    index = r.read<uint16_t>();
    verify(0 == r.read<uint16_t>());
    log_entry = r.read<uint32_t>();
    verify(1 == r.read<uint8_t>());
    script_name = r.advance_wstring();
    function_name = r.advance_wstring();
}

void VMAD_PERK_Fragment::parse(BinaryReader& r) {
    index = r.read<uint16_t>();
    unk0 = r.read<int16_t>();
    unk1 = r.read<int8_t>();
    script_name = r.advance_wstring();
    fragment_name = r.advance_wstring();
}

void VMAD_SCEN_BeginEndFragment::parse(BinaryReader& r) {
    unk = r.read<int8_t>();
    script_name = r.advance_wstring();
    fragment_name = r.advance_wstring();
}

void VMAD_SCEN_PhaseFragment::parse(BinaryReader& r) {
    unk0 = r.read<int8_t>();
    phase = r.read<uint32_t>();
    unk1 = r.read<int8_t>();
    script_name = r.advance_wstring();
    fragment_name = r.advance_wstring();
}

void NVPP_Field::parse(const uint8_t* value, size_t size) {
    BinaryReader r{ value, size };

    const auto path_count = r.read<uint32_t>();
    grow(&paths, path_count);
    for (uint32_t i = 0; i < path_count; ++i) {
        NVPP_Path path;

        auto formid_count = r.read<uint32_t>();
        grow(&path.formids, formid_count); // @TODO: reserve instead of grow
        auto formids_start = r.advance(sizeof(FormID) * formid_count);
        memcpy(path.formids.data, formids_start, sizeof(FormID) * formid_count);
        path.formids.count = formid_count;
        
        paths.push(path);
    }

    const auto node_count = r.read<uint32_t>();
    grow(&nodes, node_count);
    for (uint32_t i = 0; i < node_count; ++i) {
        NVPP_Node node;
        node.formid.value = r.read<uint32_t>();
        node.index = r.read<uint32_t>();
        nodes.push(node);
    }

    qsort(nodes.data, nodes.count, sizeof(nodes.data[0]), [](void const* aa, void const* bb) -> int {
        NVPP_Node* a = (NVPP_Node*)aa;
        NVPP_Node* b = (NVPP_Node*)bb;

        return a->index - b->index;
    });

    verify(r.now == r.end);
}

const char* ctda_operator_string(CTDA_Operator op) {
    switch (op) {
        case CTDA_Operator::Equal: return "==";
        case CTDA_Operator::NotEqual: return "!=";
        case CTDA_Operator::Greater: return ">";
        case CTDA_Operator::GreaterOrEqual: return ">=";
        case CTDA_Operator::Less: return "<";
        case CTDA_Operator::LessOrEqual: return "<=";
        default: {
            verify(false);
            return "";
        }
    }
}

const CTDA_Function* find_ctda_function(const char* name, size_t count) {
    // @TODO: Use hash table.
    for (const auto& function : CTDA_Functions) {
        auto function_count = strlen(function.name);
        if (count == function_count && memory_equals(name, function.name, count)) {
            return &function;
        }
    }
    return nullptr;
}

const ActorValue* find_actor_value(const char* name, size_t count) {
    for (const auto& value : ActorValues) {
        auto value_count = strlen(value.name);
        if (count == value_count && memory_equals(name, value.name, count)) {
            return &value;
        }
    }
    return nullptr;
}

const CTDA_Function CTDA_Functions[727] = {
    { 0  , "GetWantBlocking" },
    { 1  , "GetDistance" },
    { 2  , "AddItem" },
    { 3  , "SetEssential" },
    { 4  , "Rotate" },
    { 5  , "GetLocked" },
    { 6  , "GetPos" },
    { 7  , "SetPos" },
    { 8  , "GetAngle" },
    { 9  , "SetAngle" },
    { 10 , "GetStartingPos" },
    { 11 , "GetStartingAngle" },
    { 12 , "GetSecondsPassed" },
    { 13 , "Activate" },
    { 14 , "GetActorValue", CTDA_ArgumentType::ActorValue, CTDA_ArgumentType::None },
    { 15 , "SetActorValue" },
    { 16 , "ModActorValue" },
    { 17 , "SetAtStart" },
    { 18 , "GetCurrentTime" },
    { 19 , "PlayGroup" },
    { 20 , "LoopGroup" },
    { 21 , "SkipAnim" },
    { 22 , "StartCombat" },
    { 23 , "StopCombat" },
    { 24 , "GetScale" },
    { 25 , "IsMoving" },
    { 26 , "IsTurning" },
    { 27 , "GetLineOfSight" },
    { 28 , "AddSpell" },
    { 29 , "RemoveSpell" },
    { 30 , "Cast" },
    { 31 , "GetButtonPressed" },
    { 32 , "GetInSameCell" },
    { 33 , "Enable" },
    { 34 , "Disable" },
    { 35 , "GetDisabled" },
    { 36 , "MenuMode" },
    { 37 , "PlaceAtMe" },
    { 38 , "PlaySound" },
    { 39 , "GetDisease" },
    { 40 , "FailAllObjectives" },
    { 41 , "GetClothingValue" },
    { 42 , "SameFaction" },
    { 43 , "SameRace" },
    { 44 , "SameSex" },
    { 45 , "GetDetected" },
    { 46 , "GetDead" },
    { 47 , "GetItemCount", CTDA_ArgumentType::FormID, CTDA_ArgumentType::None },
    { 48 , "GetGold" },
    { 49 , "GetSleeping" },
    { 50 , "GetTalkedToPC" },
    { 51 , "Say" },
    { 52 , "SayTo" },
    { 53 , "GetScriptVariable" },
    { 54 , "StartQuest" },
    { 55 , "StopQuest" },
    { 56 , "GetQuestRunning" },
    { 57 , "SetStage" },
    { 58 , "GetStage", CTDA_ArgumentType::FormID, CTDA_ArgumentType::None },
    { 59 , "GetStageDone" },
    { 60 , "GetFactionRankDifference" },
    { 61 , "GetAlarmed" },
    { 62 , "IsRaining" },
    { 63 , "GetAttacked" },
    { 64 , "GetIsCreature" },
    { 65 , "GetLockLevel" },
    { 66 , "GetShouldAttack" },
    { 67 , "GetInCell" },
    { 68 , "GetIsClass" },
    { 69 , "GetIsRace" },
    { 70 , "GetIsSex" },
    { 71 , "GetInFaction", CTDA_ArgumentType::FormID, CTDA_ArgumentType::None },
    { 72 , "GetIsID", CTDA_ArgumentType::FormID, CTDA_ArgumentType::None },
    { 73 , "GetFactionRank" },
    { 74 , "GetGlobalValue" },
    { 75 , "IsSnowing" },
    { 76 , "FastTravel" },
    { 77 , "GetRandomPercent" },
    { 78 , "RemoveMusic" },
    { 79 , "GetQuestVariable" },
    { 80 , "GetLevel" },
    { 81 , "IsRotating" },
    { 82 , "RemoveItem" },
    { 83 , "GetLeveledEncounterValue" },
    { 84 , "GetDeadCount" },
    { 85 , "AddToMap" },
    { 86 , "StartConversation" },
    { 87 , "Drop" },
    { 88 , "AddTopic" },
    { 89 , "ShowMessage" },
    { 90 , "SetAlert" },
    { 91 , "GetIsAlerted" },
    { 92 , "Look" },
    { 93 , "StopLook" },
    { 94 , "EvaluatePackage" },
    { 95 , "SendAssaultAlarm" },
    { 96 , "EnablePlayerControls" },
    { 97 , "DisablePlayerControls" },
    { 98 , "GetPlayerControlsDisabled" },
    { 99 , "GetHeadingAngle" },
    { 100, "PickIdle" },
    { 101, "IsWeaponMagicOut" },
    { 102, "IsTorchOut" },
    { 103, "IsShieldOut" },
    { 104, "CreateDetectionEvent" },
    { 105, "IsActionRef" },
    { 106, "IsFacingUp" },
    { 107, "GetKnockedState" },
    { 108, "GetWeaponAnimType" },
    { 109, "IsWeaponSkillType" },
    { 110, "GetCurrentAIPackage" },
    { 111, "IsWaiting" },
    { 112, "IsIdlePlaying" },
    { 113, "CompleteQuest" },
    { 114, "Lock" },
    { 115, "UnLock" },
    { 116, "IsIntimidatedbyPlayer" },
    { 117, "IsPlayerInRegion" },
    { 118, "GetActorAggroRadiusViolated" },
    { 119, "GetCrimeKnown" },
    { 120, "SetEnemy" },
    { 121, "SetAlly" },
    { 122, "GetCrime" },
    { 123, "IsGreetingPlayer" },
    { 124, "StartMisterSandMan" },
    { 125, "IsGuard" },
    { 126, "StartCannibal" },
    { 127, "HasBeenEaten" },
    { 128, "GetStaminaPercentage" },
    { 129, "GetPCIsClass" },
    { 130, "GetPCIsRace" },
    { 131, "GetPCIsSex" },
    { 132, "GetPCInFaction" },
    { 133, "SameFactionAsPC" },
    { 134, "SameRaceAsPC" },
    { 135, "SameSexAsPC" },
    { 136, "GetIsReference" },
    { 137, "SetFactionRank" },
    { 138, "ModFactionRank" },
    { 139, "KillActor" },
    { 140, "ResurrectActor" },
    { 141, "IsTalking" },
    { 142, "GetWalkSpeed" },
    { 143, "GetCurrentAIProcedure" },
    { 144, "GetTrespassWarningLevel" },
    { 145, "IsTrespassing" },
    { 146, "IsInMyOwnedCell" },
    { 147, "GetWindSpeed" },
    { 148, "GetCurrentWeatherPercent" },
    { 149, "GetIsCurrentWeather" },
    { 150, "IsContinuingPackagePCNear" },
    { 151, "SetCrimeFaction" },
    { 152, "GetIsCrimeFaction" },
    { 153, "CanHaveFlames" },
    { 154, "HasFlames" },
    { 155, "AddFlames" },
    { 156, "RemoveFlames" },
    { 157, "GetOpenState" },
    { 158, "MoveToMarker" },
    { 159, "GetSitting" },
    { 160, "GetFurnitureMarkerID" },
    { 161, "GetIsCurrentPackage" },
    { 162, "IsCurrentFurnitureRef" },
    { 163, "IsCurrentFurnitureObj" },
    { 164, "SetSize" },
    { 165, "RemoveMe" },
    { 166, "DropMe" },
    { 167, "GetFactionReaction" },
    { 168, "SetFactionReaction" },
    { 169, "ModFactionReaction" },
    { 170, "GetDayOfWeek" },
    { 171, "IgnoreCrime" },
    { 172, "GetTalkedToPCParam" },
    { 173, "RemoveAllItems" },
    { 174, "WakeUpPC" },
    { 175, "IsPCSleeping" },
    { 176, "IsPCAMurderer" },
    { 177, "SetCombatStyle" },
    { 178, "PlaySound3D" },
    { 179, "SelectPlayerSpell" },
    { 180, "HasSameEditorLocAsRef" },
    { 181, "HasSameEditorLocAsRefAlias" },
    { 182, "GetEquipped" },
    { 183, "Wait" },
    { 184, "StopWaiting" },
    { 185, "IsSwimming" },
    { 186, "ScriptEffectElapsedSeconds" },
    { 187, "SetCellPublicFlag" },
    { 188, "GetPCSleepHours" },
    { 189, "SetPCSleepHours" },
    { 190, "GetAmountSoldStolen" },
    { 191, "ModAmountSoldStolen" },
    { 192, "GetIgnoreCrime" },
    { 193, "GetPCExpelled" },
    { 194, "SetPCExpelled" },
    { 195, "GetPCFactionMurder" },
    { 196, "SetPCFactionMurder" },
    { 197, "GetPCEnemyofFaction" },
    { 198, "SetPCEnemyofFaction" },
    { 199, "GetPCFactionAttack" },
    { 200, "SetPCFactionAttack" },
    { 201, "StartScene" },
    { 202, "StopScene" },
    { 203, "GetDestroyed" },
    { 204, "SetDestroyed" },
    { 205, "GetActionRef" },
    { 206, "GetSelf" },
    { 207, "GetContainer" },
    { 208, "GetForceRun" },
    { 209, "SetForceRun" },
    { 210, "GetForceSneak" },
    { 211, "SetForceSneak" },
    { 212, "AdvancePCSkill" },
    { 213, "AdvancePCLevel" },
    { 214, "HasMagicEffect" },
    { 215, "GetDefaultOpen" },
    { 216, "SetDefaultOpen" },
    { 217, "ShowClassMenu" },
    { 218, "ShowRaceMenu" },
    { 219, "GetAnimAction" },
    { 220, "ShowNameMenu" },
    { 221, "SetOpenState" },
    { 222, "ResetReference" },
    { 223, "IsSpellTarget" },
    { 224, "GetVATSMode" },
    { 225, "GetPersuasionNumber" },
    { 226, "GetVampireFeed" },
    { 227, "GetCannibal" },
    { 228, "GetIsClassDefault" },
    { 229, "GetClassDefaultMatch" },
    { 230, "GetInCellParam" },
    { 231, "UnusedFunction1" },
    { 232, "GetCombatTarget" },
    { 233, "GetPackageTarget" },
    { 234, "ShowSpellMaking" },
    { 235, "GetVatsTargetHeight" },
    { 236, "SetGhost" },
    { 237, "GetIsGhost" },
    { 238, "EquipItem" },
    { 239, "UnequipItem" },
    { 240, "SetClass" },
    { 241, "SetUnconscious" },
    { 242, "GetUnconscious" },
    { 243, "SetRestrained" },
    { 244, "GetRestrained" },
    { 245, "ForceFlee" },
    { 246, "GetIsUsedItem" },
    { 247, "GetIsUsedItemType" },
    { 248, "IsScenePlaying" },
    { 249, "IsInDialogueWithPlayer" },
    { 250, "GetLocationCleared" },
    { 251, "SetLocationCleared" },
    { 252, "ForceRefIntoAlias" },
    { 253, "EmptyRefAlias" },
    { 254, "GetIsPlayableRace" },
    { 255, "GetOffersServicesNow" },
    { 256, "GetGameSetting" },
    { 257, "StopCombatAlarmOnActor" },
    { 258, "HasAssociationType" },
    { 259, "HasFamilyRelationship" },
    { 260, "SetWeather" },
    { 261, "HasParentRelationship" },
    { 262, "IsWarningAbout" },
    { 263, "IsWeaponOut" },
    { 264, "HasSpell" },
    { 265, "IsTimePassing" },
    { 266, "IsPleasant" },
    { 267, "IsCloudy" },
    { 268, "TrapUpdate" },
    { 269, "ShowQuestObjectives" },
    { 270, "ForceActorValue" },
    { 271, "IncrementPCSkill" },
    { 272, "DoTrap" },
    { 273, "EnableFastTravel" },
    { 274, "IsSmallBump" },
    { 275, "GetParentRef" },
    { 276, "PlayBink" },
    { 277, "GetBaseActorValue" },
    { 278, "IsOwner" },
    { 279, "SetOwnership" },
    { 280, "IsCellOwner" },
    { 281, "SetCellOwnership" },
    { 282, "IsHorseStolen" },
    { 283, "SetCellFullName" },
    { 284, "SetActorFullName" },
    { 285, "IsLeftUp" },
    { 286, "IsSneaking" },
    { 287, "IsRunning" },
    { 288, "GetFriendHit" },
    { 289, "IsInCombat" },
    { 290, "SetPackDuration" },
    { 291, "PlayMagicShaderVisuals" },
    { 292, "PlayMagicEffectVisuals" },
    { 293, "StopMagicShaderVisuals" },
    { 294, "StopMagicEffectVisuals" },
    { 295, "ResetInterior" },
    { 296, "IsAnimPlaying" },
    { 297, "SetActorAlpha" },
    { 298, "EnableLinkedPathPoints" },
    { 299, "DisableLinkedPathPoints" },
    { 300, "IsInInterior" },
    { 301, "ForceWeather" },
    { 302, "ToggleActorsAI" },
    { 303, "IsActorsAIOff" },
    { 304, "IsWaterObject" },
    { 305, "GetPlayerAction" },
    { 306, "IsActorUsingATorch" },
    { 307, "SetLevel" },
    { 308, "ResetFallDamageTimer" },
    { 309, "IsXBox" },
    { 310, "GetInWorldspace" },
    { 311, "ModPCMiscStat" },
    { 312, "GetPCMiscStat" },
    { 313, "GetPairedAnimation" },
    { 314, "IsActorAVictim" },
    { 315, "GetTotalPersuasionNumber" },
    { 316, "SetScale" },
    { 317, "ModScale" },
    { 318, "GetIdleDoneOnce" },
    { 319, "KillAllActors" },
    { 320, "GetNoRumors" },
    { 321, "SetNoRumors" },
    { 322, "Dispel" },
    { 323, "GetCombatState" },
    { 324, "TriggerHitShader" },
    { 325, "GetWithinPackageLocation" },
    { 326, "Reset3DState" },
    { 327, "IsRidingHorse" },
    { 328, "DispelAllSpells" },
    { 329, "IsFleeing" },
    { 330, "AddAchievement" },
    { 331, "DuplicateAllItems" },
    { 332, "IsInDangerousWater" },
    { 333, "EssentialDeathReload" },
    { 334, "SetShowQuestItems" },
    { 335, "DuplicateNPCStats" },
    { 336, "ResetHealth" },
    { 337, "SetIgnoreFriendlyHits" },
    { 338, "GetIgnoreFriendlyHits" },
    { 339, "IsPlayersLastRiddenHorse" },
    { 340, "SetActorRefraction" },
    { 341, "SetItemValue" },
    { 342, "SetRigidBodyMass" },
    { 343, "ShowViewerStrings" },
    { 344, "ReleaseWeatherOverride" },
    { 345, "SetAllReachable" },
    { 346, "SetAllVisible" },
    { 347, "SetNoAvoidance" },
    { 348, "SendTrespassAlarm" },
    { 349, "SetSceneIsComplex" },
    { 350, "Autosave" },
    { 351, "StartMasterFileSeekData" },
    { 352, "DumpMasterFileSeekData" },
    { 353, "IsActor" },
    { 354, "IsEssential" },
    { 355, "PreloadMagicEffect" },
    { 356, "ShowDialogSubtitles" },
    { 357, "SetPlayerResistingArrest" },
    { 358, "IsPlayerMovingIntoNewSpace" },
    { 359, "GetInCurrentLoc" },
    { 360, "GetInCurrentLocAlias" },
    { 361, "GetTimeDead" },
    { 362, "HasLinkedRef" },
    { 363, "GetLinkedRef" },
    { 364, "DamageObject" },
    { 365, "IsChild" },
    { 366, "GetStolenItemValueNoCrime" },
    { 367, "GetLastPlayerAction" },
    { 368, "IsPlayerActionActive" },
    { 369, "SetTalkingActivatorActor" },
    { 370, "IsTalkingActivatorActor" },
    { 371, "ShowBarterMenu" },
    { 372, "IsInList" },
    { 373, "GetStolenItemValue" },
    { 374, "AddPerk" },
    { 375, "GetCrimeGoldViolent" },
    { 376, "GetCrimeGoldNonviolent" },
    { 377, "ShowRepairMenu" },
    { 378, "HasShout" },
    { 379, "AddNote" },
    { 380, "RemoveNote" },
    { 381, "GetHasNote" },
    { 382, "AddToFaction" },
    { 383, "RemoveFromFaction" },
    { 384, "DamageActorValue" },
    { 385, "RestoreActorValue" },
    { 386, "TriggerHUDShudder" },
    { 387, "GetObjectiveFailed" },
    { 388, "SetObjectiveFailed" },
    { 389, "SetGlobalTimeMultiplier" },
    { 390, "GetHitLocation" },
    { 391, "IsPC1stPerson" },
    { 392, "PurgeCellBuffers" },
    { 393, "PushActorAway" },
    { 394, "SetActorsAI" },
    { 395, "ClearOwnership" },
    { 396, "GetCauseofDeath" },
    { 397, "IsLimbGone" },
    { 398, "IsWeaponInList" },
    { 399, "PlayIdle" },
    { 400, "ApplyImageSpaceModifier" },
    { 401, "RemoveImageSpaceModifier" },
    { 402, "IsBribedbyPlayer" },
    { 403, "GetRelationshipRank" },
    { 404, "SetRelationshipRank" },
    { 405, "SetCellImageSpace" },
    { 406, "ShowChargenMenu" },
    { 407, "GetVATSValue" },
    { 408, "IsKiller" },
    { 409, "IsKillerObject" },
    { 410, "GetFactionCombatReaction" },
    { 411, "UseWeapon" },
    { 412, "EvaluateSpellConditions" },
    { 413, "ToggleMotionBlur" },
    { 414, "Exists" },
    { 415, "GetGroupMemberCount" },
    { 416, "GetGroupTargetCount" },
    { 417, "SetObjectiveCompleted" },
    { 418, "SetObjectiveDisplayed" },
    { 419, "GetObjectiveCompleted" },
    { 420, "GetObjectiveDisplayed" },
    { 421, "SetImageSpace" },
    { 422, "PipboyRadio" },
    { 423, "RemovePerk" },
    { 424, "DisableAllActors" },
    { 425, "GetIsFormType" },
    { 426, "GetIsVoiceType" },
    { 427, "GetPlantedExplosive" },
    { 428, "CompleteAllObjectives" },
    { 429, "IsScenePackageRunning" },
    { 430, "GetHealthPercentage" },
    { 431, "SetAudioMultithreading" },
    { 432, "GetIsObjectType" },
    { 433, "ShowChargenMenuParams" },
    { 434, "GetDialogueEmotion" },
    { 435, "GetDialogueEmotionValue" },
    { 436, "ExitGame" },
    { 437, "GetIsCreatureType" },
    { 438, "PlayerCreatePotion" },
    { 439, "PlayerEnchantObject" },
    { 440, "ShowWarning" },
    { 441, "EnterTrigger" },
    { 442, "MarkForDelete" },
    { 443, "SetPlayerAIDriven" },
    { 444, "GetInCurrentLocFormList" },
    { 445, "GetInZone" },
    { 446, "GetVelocity" },
    { 447, "GetGraphVariableFloat" },
    { 448, "HasPerk" },
    { 449, "GetFactionRelation" },
    { 450, "IsLastIdlePlayed" },
    { 451, "SetNPCRadio" },
    { 452, "SetPlayerTeammate" },
    { 453, "GetPlayerTeammate" },
    { 454, "GetPlayerTeammateCount" },
    { 455, "OpenActorContainer" },
    { 456, "ClearFactionPlayerEnemyFlag" },
    { 457, "ClearActorsFactionsPlayerEnemyFlag" },
    { 458, "GetActorCrimePlayerEnemy" },
    { 459, "GetCrimeGold" },
    { 460, "SetCrimeGold" },
    { 461, "ModCrimeGold" },
    { 462, "GetPlayerGrabbedRef" },
    { 463, "IsPlayerGrabbedRef" },
    { 464, "PlaceLeveledActorAtMe" },
    { 465, "GetKeywordItemCount" },
    { 466, "ShowLockpickMenu" },
    { 467, "GetBroadcastState" },
    { 468, "SetBroadcastState" },
    { 469, "StartRadioConversation" },
    { 470, "GetDestructionStage" },
    { 471, "ClearDestruction" },
    { 472, "CastImmediateOnSelf" },
    { 473, "GetIsAlignment" },
    { 474, "ResetQuest" },
    { 475, "SetQuestDelay" },
    { 476, "IsProtected" },
    { 477, "GetThreatRatio" },
    { 478, "MatchFaceGeometry" },
    { 479, "GetIsUsedItemEquipType" },
    { 480, "GetPlayerName" },
    { 481, "FireWeapon" },
    { 482, "PayCrimeGold" },
    { 483, "UnusedFunction2" },
    { 484, "MatchRace" },
    { 485, "SetPCYoung" },
    { 486, "SexChange" },
    { 487, "IsCarryable" },
    { 488, "GetConcussed" },
    { 489, "SetZoneRespawns" },
    { 490, "SetVATSTarget" },
    { 491, "GetMapMarkerVisible" },
    { 492, "ResetInventory" },
    { 493, "PlayerKnows" },
    { 494, "GetPermanentActorValue" },
    { 495, "GetKillingBlowLimb" },
    { 496, "GoToJail" },
    { 497, "CanPayCrimeGold" },
    { 498, "ServeTime" },
    { 499, "GetDaysInJail" },
    { 500, "EPAlchemyGetMakingPoison" },
    { 501, "EPAlchemyEffectHasKeyword" },
    { 502, "ShowAllMapMarkers" },
    { 503, "GetAllowWorldInteractions" },
    { 504, "ResetAI" },
    { 505, "SetRumble" },
    { 506, "SetNoActivationSound" },
    { 507, "ClearNoActivationSound" },
    { 508, "GetLastHitCritical" },
    { 509, "AddMusic" },
    { 510, "UnusedFunction3" },
    { 511, "UnusedFunction4" },
    { 512, "SetPCToddler" },
    { 513, "IsCombatTarget" },
    { 514, "TriggerScreenBlood" },
    { 515, "GetVATSRightAreaFree" },
    { 516, "GetVATSLeftAreaFree" },
    { 517, "GetVATSBackAreaFree" },
    { 518, "GetVATSFrontAreaFree" },
    { 519, "GetIsLockBroken" },
    { 520, "IsPS3" },
    { 521, "IsWin32" },
    { 522, "GetVATSRightTargetVisible" },
    { 523, "GetVATSLeftTargetVisible" },
    { 524, "GetVATSBackTargetVisible" },
    { 525, "GetVATSFrontTargetVisible" },
    { 526, "AttachAshPile" },
    { 527, "SetCriticalStage" },
    { 528, "IsInCriticalStage" },
    { 529, "RemoveFromAllFactions" },
    { 530, "GetXPForNextLevel" },
    { 531, "ShowLockpickMenuDebug" },
    { 532, "ForceSave" },
    { 533, "GetInfamy" },
    { 534, "GetInfamyViolent" },
    { 535, "GetInfamyNonViolent" },
    { 536, "UnusedFunction5" },
    { 537, "Sin" },
    { 538, "Cos" },
    { 539, "Tan" },
    { 540, "Sqrt" },
    { 541, "Log" },
    { 542, "Abs" },
    { 543, "GetQuestCompleted" },
    { 544, "UnusedFunction6" },
    { 545, "PipBoyRadioOff" },
    { 546, "AutoDisplayObjectives" },
    { 547, "IsGoreDisabled" },
    { 548, "FadeSFX" },
    { 549, "SetMinimalUse" },
    { 550, "IsSceneActionComplete" },
    { 551, "ShowQuestStages" },
    { 552, "GetSpellUsageNum" },
    { 553, "ForceRadioStationUpdate" },
    { 554, "GetActorsInHigh" },
    { 555, "HasLoaded3D" },
    { 556, "DisableAllMines" },
    { 557, "SetLastExtDoorActivated" },
    { 558, "KillQuestUpdates" },
    { 559, "IsImageSpaceActive" },
    { 560, "HasKeyword" },
    { 561, "HasRefType" },
    { 562, "LocationHasKeyword" },
    { 563, "LocationHasRefType" },
    { 564, "CreateEvent" },
    { 565, "GetIsEditorLocation" },
    { 566, "GetIsAliasRef" },
    { 567, "GetIsEditorLocAlias" },
    { 568, "IsSprinting" },
    { 569, "IsBlocking" },
    { 570, "HasEquippedSpell" },
    { 571, "GetCurrentCastingType" },
    { 572, "GetCurrentDeliveryType" },
    { 573, "EquipSpell" },
    { 574, "GetAttackState" },
    { 575, "GetAliasedRef" },
    { 576, "GetEventData" },
    { 577, "IsCloserToAThanB" },
    { 578, "EquipShout" },
    { 579, "GetEquippedShout" },
    { 580, "IsBleedingOut" },
    { 581, "UnlockWord" },
    { 582, "TeachWord" },
    { 583, "AddToContainer" },
    { 584, "GetRelativeAngle" },
    { 585, "SendAnimEvent" },
    { 586, "Shout" },
    { 587, "AddShout" },
    { 588, "RemoveShout" },
    { 589, "GetMovementDirection" },
    { 590, "IsInScene" },
    { 591, "GetRefTypeDeadCount" },
    { 592, "GetRefTypeAliveCount" },
    { 593, "ApplyHavokImpulse" },
    { 594, "GetIsFlying" },
    { 595, "IsCurrentSpell" },
    { 596, "SpellHasKeyword" },
    { 597, "GetEquippedItemType" },
    { 598, "GetLocationAliasCleared" },
    { 599, "SetLocationAliasCleared" },
    { 600, "GetLocAliasRefTypeDeadCount" },
    { 601, "GetLocAliasRefTypeAliveCount" },
    { 602, "IsWardState" },
    { 603, "IsInSameCurrentLocAsRef" },
    { 604, "IsInSameCurrentLocAsRefAlias" },
    { 605, "LocAliasIsLocation" },
    { 606, "GetKeywordDataForLocation" },
    { 607, "SetKeywordDataForLocation" },
    { 608, "GetKeywordDataForAlias" },
    { 609, "SetKeywordDataForAlias" },
    { 610, "LocAliasHasKeyword" },
    { 611, "IsNullPackageData" },
    { 612, "GetNumericPackageData" },
    { 613, "IsFurnitureAnimType" },
    { 614, "IsFurnitureEntryType" },
    { 615, "GetHighestRelationshipRank" },
    { 616, "GetLowestRelationshipRank" },
    { 617, "HasAssociationTypeAny" },
    { 618, "HasFamilyRelationshipAny" },
    { 619, "GetPathingTargetOffset" },
    { 620, "GetPathingTargetAngleOffset" },
    { 621, "GetPathingTargetSpeed" },
    { 622, "GetPathingTargetSpeedAngle" },
    { 623, "GetMovementSpeed" },
    { 624, "GetInContainer" },
    { 625, "IsLocationLoaded" },
    { 626, "IsLocAliasLoaded" },
    { 627, "IsDualCasting" },
    { 628, "DualCast" },
    { 629, "GetVMQuestVariable" },
    { 630, "GetVMScriptVariable" },
    { 631, "IsEnteringInteractionQuick" },
    { 632, "IsCasting" },
    { 633, "GetFlyingState" },
    { 634, "SetFavorState" },
    { 635, "IsInFavorState" },
    { 636, "HasTwoHandedWeaponEquipped" },
    { 637, "IsExitingInstant" },
    { 638, "IsInFriendStatewithPlayer" },
    { 639, "GetWithinDistance" },
    { 640, "GetActorValuePercent" },
    { 641, "IsUnique" },
    { 642, "GetLastBumpDirection" },
    { 643, "CameraShake" },
    { 644, "IsInFurnitureState" },
    { 645, "GetIsInjured" },
    { 646, "GetIsCrashLandRequest" },
    { 647, "GetIsHastyLandRequest" },
    { 648, "UpdateQuestInstanceGlobal" },
    { 649, "SetAllowFlying" },
    { 650, "IsLinkedTo" },
    { 651, "GetKeywordDataForCurrentLocation" },
    { 652, "GetInSharedCrimeFaction" },
    { 653, "GetBribeAmount" },
    { 654, "GetBribeSuccess" },
    { 655, "GetIntimidateSuccess" },
    { 656, "GetArrestedState" },
    { 657, "GetArrestingActor" },
    { 658, "ClearArrestState" },
    { 659, "EPTemperingItemIsEnchanted" },
    { 660, "EPTemperingItemHasKeyword" },
    { 661, "GetReceivedGiftValue" },
    { 662, "GetGiftGivenValue" },
    { 663, "ForceLocIntoAlias" },
    { 664, "GetReplacedItemType" },
    { 665, "SetHorseActor" },
    { 666, "PlayReferenceEffect" },
    { 667, "StopReferenceEffect" },
    { 668, "PlayShaderParticleGeometry" },
    { 669, "StopShaderParticleGeometry" },
    { 670, "ApplyImageSpaceModifierCrossFade" },
    { 671, "RemoveImageSpaceModifierCrossFade" },
    { 672, "IsAttacking" },
    { 673, "IsPowerAttacking" },
    { 674, "IsLastHostileActor" },
    { 675, "GetGraphVariableInt" },
    { 676, "GetCurrentShoutVariation" },
    { 677, "PlayImpactEffect" },
    { 678, "ShouldAttackKill" },
    { 679, "SendStealAlarm" },
    { 680, "GetActivationHeight" },
    { 681, "EPModSkillUsage_IsAdvanceSkill" },
    { 682, "WornHasKeyword" },
    { 683, "GetPathingCurrentSpeed" },
    { 684, "GetPathingCurrentSpeedAngle" },
    { 685, "KnockAreaEffect" },
    { 686, "InterruptCast" },
    { 687, "AddFormToFormList" },
    { 688, "RevertFormList" },
    { 689, "AddFormToLeveledList" },
    { 690, "RevertLeveledList" },
    { 691, "EPModSkillUsage_AdvanceObjectHasKeyword" },
    { 692, "EPModSkillUsage_IsAdvanceAction" },
    { 693, "EPMagic_SpellHasKeyword" },
    { 694, "GetNoBleedoutRecovery" },
    { 695, "SetNoBleedoutRecovery" },
    { 696, "EPMagic_SpellHasSkill" },
    { 697, "IsAttackType" },
    { 698, "IsAllowedToFly" },
    { 699, "HasMagicEffectKeyword" },
    { 700, "IsCommandedActor" },
    { 701, "IsStaggered" },
    { 702, "IsRecoiling" },
    { 703, "IsExitingInteractionQuick" },
    { 704, "IsPathing" },
    { 705, "GetShouldHelp" },
    { 706, "HasBoundWeaponEquipped" },
    { 707, "GetCombatTargetHasKeyword" },
    { 708, "UpdateLevel" },
    { 709, "GetCombatGroupMemberCount" },
    { 710, "IsIgnoringCombat" },
    { 711, "GetLightLevel" },
    { 712, "SavePCFace" },
    { 713, "SpellHasCastingPerk" },
    { 714, "IsBeingRidden" },
    { 715, "IsUndead" },
    { 716, "GetRealHoursPassed" },
    { 717, "UnequipAll" },
    { 718, "IsUnlockedDoor" },
    { 719, "IsHostileToActor" },
    { 720, "GetTargetHeight" },
    { 721, "IsPoison" },
    { 722, "WornApparelHasKeywordCount" },
    { 723, "GetItemHealthPercent" },
    { 724, "EffectWasDualCast" },
    { 725, "GetKnockStateEnum" },
    { 726, "DoesNotExist" },
};

const ActorValue ActorValues[164]{
    { 0  , "Aggression" },
    { 1  , "Confidence" },
    { 2  , "Energy" },
    { 3  , "Morality" },
    { 4  , "Mood" },
    { 5  , "Assistance" },
    { 6  , "OneHanded" },
    { 7  , "TwoHanded" },
    { 8  , "Marksman" },
    { 9  , "Block" },
    { 10 , "Smithing" },
    { 11 , "HeavyArmor" },
    { 12 , "LightArmor" },
    { 13 , "Pickpocket" },
    { 14 , "Lockpicking" },
    { 15 , "Sneak" },
    { 16 , "Alchemy" },
    { 17 , "Speechcraft" },
    { 18 , "Alteration" },
    { 19 , "Conjuration" },
    { 20 , "Destruction" },
    { 21 , "Illusion" },
    { 22 , "Restoration" },
    { 23 , "Enchanting" },
    { 24 , "Health" },
    { 25 , "Magicka" },
    { 26 , "Stamina" },
    { 27 , "HealRate" },
    { 28 , "MagickaRate" },
    { 29 , "StaminaRate" },
    { 30 , "SpeedMult" },
    { 31 , "InventoryWeight" },
    { 32 , "CarryWeight" },
    { 33 , "CritChance" },
    { 34 , "MeleeDamage" },
    { 35 , "UnarmedDamage" },
    { 36 , "Mass" },
    { 37 , "VoicePoints" },
    { 38 , "VoiceRate" },
    { 39 , "DamageResist" },
    { 40 , "PoisonResist" },
    { 41 , "FireResist" },
    { 42 , "ElectricResist" },
    { 43 , "FrostResist" },
    { 44 , "MagicResist" },
    { 45 , "DiseaseResist" },
    { 46 , "PerceptionCondition" },
    { 47 , "EnduranceCondition" },
    { 48 , "LeftAttackCondition" },
    { 49 , "RightAttackCondition" },
    { 50 , "LeftMobilityCondition" },
    { 51 , "RightMobilityCondition" },
    { 52 , "BrainCondition" },
    { 53 , "Paralysis" },
    { 54 , "Invisibility" },
    { 55 , "NightEye" },
    { 56 , "DetectLifeRange" },
    { 57 , "WaterBreathing" },
    { 58 , "WaterWalking" },
    { 59 , "IgnoreCrippledLimbs" },
    { 60 , "Fame" },
    { 61 , "Infamy" },
    { 62 , "JumpingBonus" },
    { 63 , "WardPower" },
    { 64 , "RightItemCharge/EquippedItemCharge" },
    { 65 , "ArmorPerks" },
    { 66 , "ShieldPerks" },
    { 67 , "WardDeflection" },
    { 68 , "Variable01" },
    { 69 , "Variable02" },
    { 70 , "Variable03" },
    { 71 , "Variable04" },
    { 72 , "Variable05" },
    { 73 , "Variable06" },
    { 74 , "Variable07" },
    { 75 , "Variable08" },
    { 76 , "Variable09" },
    { 77 , "Variable10" },
    { 78 , "BowSpeedBonus" },
    { 79 , "FavorActive" },
    { 80 , "FavorsPerDay" },
    { 81 , "FavorsPerDayTimer" },
    { 82 , "LeftItemCharge/EquippedStaffCharge" },
    { 83 , "AbsorbChance" },
    { 84 , "Blindness" },
    { 85 , "WeaponSpeedMult" },
    { 86 , "ShoutRecoveryMult" },
    { 87 , "BowStaggerBonus" },
    { 88 , "Telekinesis" },
    { 89 , "FavorPointsBonus" },
    { 90 , "LastBribedIntimidated" },
    { 91 , "LastFlattered" },
    { 92 , "MovementNoiseMult" },
    { 93 , "BypassVendorStolenCheck" },
    { 94 , "BypassVendorKeywordCheck" },
    { 95 , "WaitingForPlayer" },
    { 96 , "OneHandedMod" },
    { 97 , "TwoHandedMod" },
    { 98 , "MarksmanMod" },
    { 99 , "BlockMod" },
    { 100, "SmithingMod" },
    { 101, "HeavyArmorMod" },
    { 102, "LightArmorMod" },
    { 103, "PickPocketMod" },
    { 104, "LockpickingMod" },
    { 105, "SneakMod" },
    { 106, "AlchemyMod" },
    { 107, "SpeechcraftMod" },
    { 108, "AlterationMod" },
    { 109, "ConjurationMod" },
    { 110, "DestructionMod" },
    { 111, "IllusionMod" },
    { 112, "RestorationMod" },
    { 113, "EnchantingMod" },
    { 114, "OneHandedSkillAdvance" },
    { 115, "TwoHandedSkillAdvance" },
    { 116, "MarksmanSkillAdvance" },
    { 117, "BlockSkillAdvance" },
    { 118, "SmithingSkillAdvance" },
    { 119, "HeavyArmorSkillAdvance" },
    { 120, "LightArmorSkillAdvance" },
    { 121, "PickPocketSkillAdvance" },
    { 122, "LockpickingSkillAdvance" },
    { 123, "SneakSkillAdvance" },
    { 124, "AlchemySkillAdvance" },
    { 125, "SpeechcraftSkillAdvance" },
    { 126, "AlterationSkillAdvance" },
    { 127, "ConjurationSkillAdvance" },
    { 128, "DestructionSkillAdvance" },
    { 129, "IllusionSkillAdvance" },
    { 130, "RestorationSkillAdvance" },
    { 131, "EnchantingSkillAdvance" },
    { 132, "LeftWeaponSpeedMult" },
    { 133, "DragonSouls" },
    { 134, "CombatHealthRegenMult" },
    { 135, "OneHandedPowerMod" },
    { 136, "TwoHandedPowerMod" },
    { 137, "MarksmanPowerMod" },
    { 138, "BlockPowerMod" },
    { 139, "SmithingPowerMod" },
    { 140, "HeavyArmorPowerMod" },
    { 141, "LightArmorPowerMod" },
    { 142, "PickPocketPowerMod" },
    { 143, "LockpickingPowerMod" },
    { 144, "SneakPowerMod" },
    { 145, "AlchemyPowerMod" },
    { 146, "SpeechcraftPowerMod" },
    { 147, "AlterationPowerMod" },
    { 148, "ConjurationPowerMod" },
    { 149, "DestructionPowerMod" },
    { 150, "IllusionPowerMod" },
    { 151, "RestorationPowerMod" },
    { 152, "EnchantingPowerMod" },
    { 153, "DragonRend" },
    { 154, "AttackDamageMult" },
    { 155, "HealRateMult/CombatHealthRegenMultMod" },
    { 156, "MagickaRateMult/CombatHealthRegenMultPowerMod" },
    { 157, "StaminaRateMult" },
    { 158, "WerewolfPerks" },
    { 159, "VampirePerks" },
    { 160, "GrabActorOffset" },
    { 161, "Grabbed" },
    { 162, "DEPRECATED05" },
    { 163, "ReflectDamage" }
};
