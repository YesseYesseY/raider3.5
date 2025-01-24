#pragma once

#include "../UE4.h"
#include "Spawners.h"

// Pretty sure most of this needs a rework
namespace Inventory
{
    static void Update(SDK::AFortPlayerController* Controller, int Dirty = 0, bool bRemovedItem = false)
    {
        Controller->WorldInventory->bRequiresLocalUpdate = true;
        Controller->WorldInventory->HandleInventoryLocalUpdate();
        Controller->WorldInventory->ForceNetUpdate();
        Controller->HandleWorldInventoryLocalUpdate();

        Controller->QuickBars->OnRep_PrimaryQuickBar();
        Controller->QuickBars->OnRep_SecondaryQuickBar();
        Controller->QuickBars->ForceNetUpdate();
        Controller->ForceUpdateQuickbar(SDK::EFortQuickBars::Primary);
        Controller->ForceUpdateQuickbar(SDK::EFortQuickBars::Secondary);

        if (bRemovedItem)
            Controller->WorldInventory->Inventory.MarkArrayDirty();

        if (Dirty != 0 && Controller->WorldInventory->Inventory.ReplicatedEntries.Num() >= Dirty)
            Controller->WorldInventory->Inventory.MarkItemDirty(Controller->WorldInventory->Inventory.ReplicatedEntries[Dirty]);
    }

    static bool IsValidGuid(SDK::AFortPlayerControllerAthena* Controller, const SDK::FGuid& Guid)
    {
        auto& QuickBarSlots = Controller->QuickBars->PrimaryQuickBar.Slots;

        for (int i = 0; i < QuickBarSlots.Num(); i++)
        {
            if (QuickBarSlots[i].Items.Data)
            {
                auto items = QuickBarSlots[i].Items;

                for (int i = 0; items.Num(); i++)
                {
                    if (items[i] == Guid)
                        return true;
                }
            }
        }

        return false;
    }

    static SDK::FFortItemEntry GetEntryInSlot(SDK::AFortPlayerControllerAthena* Controller, int Slot, int Item = 0, SDK::EFortQuickBars QuickBars = SDK::EFortQuickBars::Primary)
    {
        auto ret = SDK::FFortItemEntry();

        auto& ItemInstances = Controller->WorldInventory->Inventory.ItemInstances;

        SDK::FGuid ToFindGuid;

        if (QuickBars == SDK::EFortQuickBars::Primary)
        {
            ToFindGuid = Controller->QuickBars->PrimaryQuickBar.Slots[Slot].Items[Item];
        }

        else if (QuickBars == SDK::EFortQuickBars::Secondary)
        {
            ToFindGuid = Controller->QuickBars->SecondaryQuickBar.Slots[Slot].Items[Item];
        }

        for (int j = 0; j < ItemInstances.Num(); j++)
        {
            auto ItemInstance = ItemInstances[j];

            if (!ItemInstance)
                continue;

            auto Guid = ItemInstance->ItemEntry.ItemGuid;

            if (ToFindGuid == Guid)
                ret = ItemInstance->ItemEntry;
        }

        return ret;
    }

    inline auto GetDefinitionInSlot(SDK::AFortPlayerControllerAthena* Controller, int Slot, int Item = 0, SDK::EFortQuickBars QuickBars = SDK::EFortQuickBars::Primary)
    {
        return GetEntryInSlot(Controller, Slot, Item, QuickBars).ItemDefinition;
    }

    static SDK::UFortWorldItem* GetInstanceFromGuid(SDK::AController* PC, const SDK::FGuid& ToFindGuid, int* index = nullptr)
    {
        auto& ItemInstances = static_cast<SDK::AFortPlayerController*>(PC)->WorldInventory->Inventory.ItemInstances;

        for (int j = 0; j < ItemInstances.Num(); j++)
        {
            auto ItemInstance = ItemInstances[j];

            if (!ItemInstance)
                continue;

            auto Guid = ItemInstance->ItemEntry.ItemGuid;

            if (ToFindGuid == Guid)
            {
                if (index)
                    *index = j;
                return ItemInstance;
            }
        }

        return nullptr;
    }

    static auto AddItemToSlot(SDK::AFortPlayerControllerAthena* Controller, SDK::UFortWorldItemDefinition* Definition, int Slot, SDK::EFortQuickBars Bars = SDK::EFortQuickBars::Primary, int Count = 1, int* Idx = nullptr)
    {
        auto ret = SDK::FFortItemEntry();

        if (Definition->IsA(SDK::UFortWeaponItemDefinition::StaticClass()))
        {
            Count = 1;
        }

        if (Slot < 0)
        {
            Slot = 1;
        }

        if (Bars == SDK::EFortQuickBars::Primary && Slot >= 6)
        {
            Slot = 5;
        }

        if (auto TempItemInstance = static_cast<SDK::UFortWorldItem*>(Definition->CreateTemporaryItemInstanceBP(Count, 1)))
        {
            TempItemInstance->SetOwningControllerForTemporaryItem(Controller);

            TempItemInstance->ItemEntry.Count = Count;
            TempItemInstance->OwnerInventory = Controller->WorldInventory;

            auto& ItemEntry = TempItemInstance->ItemEntry;

            auto _Idx = Controller->WorldInventory->Inventory.ReplicatedEntries.Add(ItemEntry);

            if (Idx)
                *Idx = _Idx;

            Controller->WorldInventory->Inventory.ItemInstances.Add(TempItemInstance);
            Controller->QuickBars->ServerAddItemInternal(ItemEntry.ItemGuid, Bars, Slot);

            Update(Controller, _Idx);

            ret = ItemEntry;
        }

        return ret;
    }

    static bool RemoveItemFromSlot(SDK::AFortPlayerControllerAthena* Controller, int Slot, SDK::EFortQuickBars Quickbars = SDK::EFortQuickBars::Primary, int Amount = -1) // -1 for all items in the slot
    {
        auto& PrimarySlots = Controller->QuickBars->PrimaryQuickBar.Slots;
        auto& SecondarySlots = Controller->QuickBars->SecondaryQuickBar.Slots;

        bool bPrimaryQuickBar = (Quickbars == SDK::EFortQuickBars::Primary);

        if (Amount == -1)
        {
            if (bPrimaryQuickBar)
                Amount = PrimarySlots[Slot].Items.Num();
            else
                Amount = SecondarySlots[Slot].Items.Num();
        }

        bool bWasSuccessful = false;

        for (int i = 0; i < Amount; i++)
        {
            // todo add a check to make sure the slot has that amount of items
            auto& ToRemoveGuid = bPrimaryQuickBar ? PrimarySlots[Slot].Items[i] : SecondarySlots[Slot].Items[i];

            for (int j = 0; j < Controller->WorldInventory->Inventory.ItemInstances.Num(); j++)
            {
                auto ItemInstance = Controller->WorldInventory->Inventory.ItemInstances[j];

                if (!ItemInstance)
                    continue;

                auto Guid = ItemInstance->ItemEntry.ItemGuid;

                if (ToRemoveGuid == Guid)
                {
                    Controller->WorldInventory->Inventory.ItemInstances.RemoveAt(j);
                    bWasSuccessful = true;
                    // break;
                }
            }

            for (int x = 0; x < Controller->WorldInventory->Inventory.ReplicatedEntries.Num(); x++)
            {
                auto& ItemEntry = Controller->WorldInventory->Inventory.ReplicatedEntries[x];

                if (ItemEntry.ItemGuid == ToRemoveGuid)
                {
                    Controller->WorldInventory->Inventory.ReplicatedEntries.RemoveAt(x);
                    bWasSuccessful = true;

                    // break;
                }
            }

            Controller->QuickBars->ServerRemoveItemInternal(ToRemoveGuid, false, true);
            ToRemoveGuid.Reset();
        }

        if (bWasSuccessful) // Make sure it acutally removed from the ItemInstances and ReplicatedEntries
        {
            Controller->QuickBars->EmptySlot(Quickbars, Slot);

            if (bPrimaryQuickBar)
                PrimarySlots[Slot].Items.FreeArray();
            else
                SecondarySlots[Slot].Items.FreeArray();

            // bPrimaryQuickBar ? PrimarySlots[Slot].Items.FreeArray() : SecondarySlots[Slot].Items.FreeArray();
        }

        Update(Controller, 0, true);

        return bWasSuccessful;
    }

    static bool IsGuidInInventory(SDK::AFortPlayerControllerAthena* Controller, const SDK::FGuid& Guid)
    {
        auto& QuickBarSlots = Controller->QuickBars->PrimaryQuickBar.Slots;

        for (int i = 0; i < QuickBarSlots.Num(); i++)
        {
            if (QuickBarSlots[i].Items.Data)
            {
                auto items = QuickBarSlots[i].Items;

                for (int i = 0; items.Num(); i++)
                {
                    if (items[i] == Guid)
                        return true;
                }
            }
        }

        return false;
    }

    static SDK::AFortWeapon* EquipWeaponDefinition(SDK::APawn* dPawn, SDK::UFortWeaponItemDefinition* Definition, const SDK::FGuid& Guid, int Ammo = -1, bool bEquipWithMaxAmmo = false) // don't use, use EquipInventoryItem // not too secure
    {
        SDK::AFortWeapon* Weapon = nullptr;

        auto weaponClass = Definition->GetWeaponActorClass();
        auto Pawn = static_cast<SDK::APlayerPawn_Athena_C*>(dPawn);

        if (Pawn && Definition && weaponClass)
        {
            auto Controller = static_cast<SDK::AFortPlayerControllerAthena*>(Pawn->Controller);
            auto Instance = GetInstanceFromGuid(Controller, Guid);

            if (!Inventory::IsGuidInInventory(Controller, Guid))
                return Weapon;

            if (weaponClass == SDK::ATrapTool_C::StaticClass() || weaponClass == SDK::ATrapTool_ContextTrap_Athena_C::StaticClass() || weaponClass == SDK::AFortDecoTool_ContextTrap::StaticClass())
            {
                Weapon = static_cast<SDK::AFortWeapon*>(Spawners::SpawnActor(weaponClass, {}, Pawn)); // Other people can't see their weapon.

                if (Weapon)
                {
                    Weapon->bReplicates = true;
                    Weapon->bOnlyRelevantToOwner = false;
                    
                    if (Definition->IsA(SDK::UFortContextTrapItemDefinition::StaticClass()))
                    {
                        static_cast<SDK::AFortDecoTool_ContextTrap*>(Weapon)->ContextTrapItemDefinition = (SDK::UFortContextTrapItemDefinition*)Definition;
                    }
                    static_cast<SDK::AFortTrapTool*>(Weapon)->ItemDefinition = Definition;
                }
            }
            else
            {
                Weapon = Pawn->EquipWeaponDefinition(Definition, Guid);
            }

            if (Weapon)
            {
                Weapon->WeaponData = Definition;
                Weapon->ItemEntryGuid = Guid;

                if (bEquipWithMaxAmmo)
                {
                    Weapon->AmmoCount = Weapon->GetBulletsPerClip();
                }
                else if (Ammo != -1)
                {
                    Weapon->AmmoCount = Instance->ItemEntry.LoadedAmmo;
                }

                Instance->ItemEntry.LoadedAmmo = Weapon->AmmoCount;

                Weapon->SetOwner(dPawn);
                Weapon->OnRep_ReplicatedWeaponData();
                Weapon->OnRep_AmmoCount();
                Weapon->ClientGivenTo(Pawn);
                Pawn->ClientInternalEquipWeapon(Weapon);
                Pawn->OnRep_CurrentWeapon(); // i dont think this is needed but alr
            }
        }

        return Weapon;
    }

    static void EquipInventoryItem(SDK::AFortPlayerControllerAthena* PC, SDK::FGuid& Guid)
    {
        if (!PC || PC->IsInAircraft())
            return;

        auto& ItemInstances = PC->WorldInventory->Inventory.ItemInstances;

        for (int i = 0; i < ItemInstances.Num(); i++)
        {
            auto CurrentItemInstance = ItemInstances[i];

            if (!CurrentItemInstance)
                continue;

            auto Def = static_cast<SDK::UFortWeaponItemDefinition*>(CurrentItemInstance->GetItemDefinitionBP());

            if (CurrentItemInstance->GetItemGuid() == Guid && Def)
            {
                EquipWeaponDefinition(PC->Pawn, Def, Guid); // CurrentItemInstance->ItemEntry.LoadedAmmo);
                break;
            }
        }
    }

    void EquipLoadout(SDK::AFortPlayerControllerAthena* Controller, PlayerLoadout WIDS)
    {
        SDK::FFortItemEntry pickaxeEntry;

        for (int i = 0; i < WIDS.size(); i++)
        {
            auto Def = WIDS[i];

            if (Def)
            {
                // if (i <= 1)
                // {
                    auto entry = AddItemToSlot(Controller, Def, i);
                    EquipWeaponDefinition(Controller->Pawn, Def, entry.ItemGuid, -1, true); // kms

                    if (i == 0)
                        pickaxeEntry = entry;
                // }
                // else
                // {
                //     Spawners::SummonPickup((AFortPlayerPawn*)Controller->Pawn, Def, 1, Controller->Pawn->K2_GetActorLocation());
                // }
            }
        }

        EquipInventoryItem(Controller, pickaxeEntry.ItemGuid);
    }

    inline void PickupAnim(SDK::AFortPawn* Pawn, SDK::AFortPickup* Pickup, float FlyTime = 0.40f)
    {
        Pickup->ForceNetUpdate();
        Pickup->PickupLocationData.PickupTarget = Pawn;
        Pickup->PickupLocationData.FlyTime = FlyTime;
        Pickup->PickupLocationData.ItemOwner = Pawn;
        Pickup->OnRep_PickupLocationData();

        Pickup->bPickedUp = true;
        Pickup->OnRep_bPickedUp();
    }

    static bool CanPickup(SDK::AFortPlayerControllerAthena* Controller, SDK::AFortPickup* Pickup)
    {
        if (Pickup->PawnWhoDroppedPickup == Controller->Pawn)
            return false;

        if (Pickup->bActorIsBeingDestroyed || Pickup->bPickedUp)
            return false;

        for (int i = 0; i < Controller->WorldInventory->Inventory.ItemInstances.Count; i++)
        {
            auto Instance = Controller->WorldInventory->Inventory.ItemInstances[i];
            auto ItemDef = Instance->ItemEntry.ItemDefinition;

            if (ItemDef != Pickup->PrimaryPickupItemEntry.ItemDefinition)
                continue;

            if (Instance->ItemEntry.Count >= ItemDef->MaxStackSize)
                return false;
        }

        return true;
    }

    template <typename Class>
    static SDK::FFortItemEntry FindItemInInventory(SDK::AFortPlayerControllerAthena* PC, bool& bFound)
    {
        auto ret = SDK::FFortItemEntry();

        auto& ItemInstances = PC->WorldInventory->Inventory.ItemInstances;

        bFound = false;

        for (int i = 0; i < ItemInstances.Num(); i++)
        {
            auto ItemInstance = ItemInstances[i];

            if (!ItemInstance)
                continue;

            auto Def = ItemInstance->ItemEntry.ItemDefinition;

            if (Def && Def->IsA(Class::StaticClass()))
            {
                bFound = true;
                ret = ItemInstance->ItemEntry;
            }
        }

        return ret;
    }

    static void EquipNextValidSlot(SDK::AFortPlayerControllerAthena* PlayerController)
    {
        /*bool Success = false;
        for (int i = PlayerController->QuickBars->PrimaryQuickBar.CurrentFocusedSlot + 1; i < PlayerController->QuickBars->PrimaryQuickBar.Slots.Count; i++)
        {
            auto Slot = PlayerController->QuickBars->PrimaryQuickBar.Slots[i];
            if (Slot.Items.Count > 0)
            {
                Success = true;
                EquipInventoryItem(PlayerController, Slot.Items[0]);
                PlayerController->QuickBars->PrimaryQuickBar.CurrentFocusedSlot = i;
            }
        }

        if (!Success)
        {*/
        EquipInventoryItem(PlayerController, PlayerController->QuickBars->PrimaryQuickBar.Slots[0].Items[0]);
        PlayerController->QuickBars->PrimaryQuickBar.CurrentFocusedSlot = 0;
        //}


    }

    static bool TryDeleteItem(SDK::AFortPlayerControllerAthena* PlayerController, int Index)
    {
        auto guid = PlayerController->WorldInventory->Inventory.ItemInstances[Index]->GetItemGuid();
        PlayerController->WorldInventory->Inventory.ItemInstances.RemoveAt(Index);
        PlayerController->WorldInventory->Inventory.ReplicatedEntries.RemoveAt(Index);
        int slot = -1;
        bool primary = false;
        for (int i = 0; i < PlayerController->QuickBars->PrimaryQuickBar.Slots.Count; i++)
        {
            auto qbslot = PlayerController->QuickBars->PrimaryQuickBar.Slots[i];

            for (int j = 0; j < qbslot.Items.Count; j++)
            {
                auto qbguid = qbslot.Items[j];

                if (qbguid == guid)
                {
                    slot = i;
                    primary = true;
                }
            }

            if (slot != -1)
                break;
        }
        for (int i = 0; i < PlayerController->QuickBars->SecondaryQuickBar.Slots.Count; i++)
        {
            auto qbslot = PlayerController->QuickBars->SecondaryQuickBar.Slots[i];

            for (int j = 0; j < qbslot.Items.Count; j++)
            {
                auto qbguid = qbslot.Items[j];

                if (qbguid == guid)
                {
                    slot = i;
                    primary = false;
                }
            }

            if (slot != -1)
                break;
        }
        
        if (slot != -1)
        {
            PlayerController->QuickBars->ServerRemoveItemInternal(guid, false, true);
            PlayerController->QuickBars->EmptySlot(primary ? SDK::EFortQuickBars::Primary : SDK::EFortQuickBars::Secondary, slot);
            if (primary)
                PlayerController->QuickBars->PrimaryQuickBar.Slots[slot].Items.FreeArray();
            else
                PlayerController->QuickBars->SecondaryQuickBar.Slots[slot].Items.FreeArray();
        }
        else
        {
            LOG_WARN("Quickbar slot not found in TryDeleteItem");
        }
        
        if (slot == PlayerController->QuickBars->PrimaryQuickBar.CurrentFocusedSlot)
            EquipNextValidSlot(PlayerController);

        Update(PlayerController, Index, true);
        return true;
    }

    static bool TryRemoveItem(SDK::AFortPlayerControllerAthena* PlayerController, SDK::FGuid Guid, int AmountToRemove)
    {
        int index = 0;
        auto instance = GetInstanceFromGuid(PlayerController, Guid, &index);
        if (!instance)
            return false;
        auto finalcount = instance->ItemEntry.Count - AmountToRemove;
        if (finalcount > 0)
        {
            instance->ItemEntry.Count = finalcount;
            PlayerController->WorldInventory->Inventory.ReplicatedEntries[index].Count = finalcount;
        }
        else
        {
            if (!TryDeleteItem(PlayerController, index))
            {
                LOG_ERROR("Failed to remove item entry {}", index);
                return false;
            }
        }
        Update(PlayerController, index);
        return true;
    }

    static bool TryRemoveItem(SDK::AFortPlayerControllerAthena* PlayerController, SDK::UFortItemDefinition* ItemDef, int AmountToRemove, bool focusedPrio = true)
    {
        auto& ItemInstances = PlayerController->WorldInventory->Inventory.ItemInstances;

        if (!ItemDef)
            return false;
        bool success = false;

        SDK::FGuid FocusedGuid = SDK::FGuid();
        if (focusedPrio)
        {
            auto FocusedSlotIndex = PlayerController->QuickBars->PrimaryQuickBar.CurrentFocusedSlot;

            if (FocusedSlotIndex >= 0 && FocusedSlotIndex < PlayerController->QuickBars->PrimaryQuickBar.Slots.Count)
            {
                auto FocusedGuidTemp = PlayerController->QuickBars->PrimaryQuickBar.Slots[FocusedSlotIndex].Items[0];
                auto FocusedInstance = GetInstanceFromGuid(PlayerController, FocusedGuidTemp);
                if (FocusedInstance && FocusedInstance->ItemEntry.ItemDefinition == ItemDef)
                {
                    FocusedGuid = FocusedGuidTemp;
                }
            }
        }

        if (FocusedGuid != SDK::FGuid())
        {
            success = TryRemoveItem(PlayerController, FocusedGuid, AmountToRemove);
        }
        else
        {
            for (int i = 0; i < ItemInstances.Num(); i++)
            {
                auto ItemInstance = ItemInstances[i];

                if (!ItemInstance)
                    continue;

                if (ItemInstance->ItemEntry.ItemDefinition && ItemInstance->ItemEntry.ItemDefinition == ItemDef)
                    success = TryRemoveItem(PlayerController, ItemInstance->ItemEntry.ItemGuid, AmountToRemove);
            }
        }
        return success;
    }

    static bool OnDrop(SDK::AFortPlayerControllerAthena* Controller, SDK::FGuid ItemGuid, int Count)
    {
        if (!Controller)
            return false;

        auto Instance = GetInstanceFromGuid(Controller, ItemGuid);
        if (!Instance)
            return false;
        auto LoadedAmmo = Instance->GetLoadedAmmo();
        auto Definition = Instance->ItemEntry.ItemDefinition;

        if (Count == -1)
            Count = Instance->ItemEntry.Count;

        bool bWasSuccessful = TryRemoveItem(Controller, ItemGuid, Count);

        if (bWasSuccessful)
        {
            auto Pickup = Spawners::SummonPickup(static_cast<SDK::AFortPlayerPawn*>(Controller->Pawn), Definition, Count, Controller->Pawn->K2_GetActorLocation());
            Pickup->PrimaryPickupItemEntry.LoadedAmmo = LoadedAmmo;
            Pickup->PawnWhoDroppedPickup = (SDK::AFortPlayerPawn*)Controller->Pawn;
        }
        //
        //if (bWasSuccessful && Controller->QuickBars->PrimaryQuickBar.Slots[0].Items.Data)
        //    EquipInventoryItem(Controller, Controller->QuickBars->PrimaryQuickBar.Slots[0].Items[0]); // just select pickaxe for now

        return bWasSuccessful;
    }

    inline void OnPickup(SDK::AFortPlayerControllerAthena* Controller, void* params)
    {
        auto Params = static_cast<SDK::AFortPlayerPawn_ServerHandlePickup_Params*>(params);

        if (!Controller || !Params)
            return;

        auto& ItemInstances = Controller->WorldInventory->Inventory.ItemInstances;

        if (Params->Pickup)
        {
            bool bCanGoInSecondary = true; // there is no way this is how you do it // todo: rename

            if (Params->Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(SDK::UFortWeaponItemDefinition::StaticClass()) && !Params->Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(SDK::UFortDecoItemDefinition::StaticClass()))
                bCanGoInSecondary = false;

            auto WorldItemDefinition = static_cast<SDK::UFortWorldItemDefinition*>(Params->Pickup->PrimaryPickupItemEntry.ItemDefinition);

            bool Success = false;
            for (int i = 0; i < ItemInstances.Count; i++)
            {
                if (ItemInstances[i]->ItemEntry.ItemDefinition == WorldItemDefinition)
                {
                    if (ItemInstances[i]->ItemEntry.Count >= WorldItemDefinition->MaxStackSize)
                    {
                        if (WorldItemDefinition->bAllowMultipleStacks)
                            continue;
                    }

                    auto newcount = ItemInstances[i]->ItemEntry.Count + Params->Pickup->PrimaryPickupItemEntry.Count;
                    if (newcount > WorldItemDefinition->MaxStackSize)
                    {
                        auto leftover = newcount - WorldItemDefinition->MaxStackSize;
                        Spawners::SummonPickup(static_cast<SDK::APlayerPawn_Athena_C*>(Controller->Pawn), WorldItemDefinition, leftover, Controller->Pawn->K2_GetActorLocation());
                        newcount = WorldItemDefinition->MaxStackSize;
                    }

                    Controller->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.Count = newcount;
                    Controller->WorldInventory->Inventory.ReplicatedEntries[i].Count = newcount;

                    Update(Controller, i);
                    Success = true;
                    break;
                }
            }

            if (!Success)
            {
                if (!bCanGoInSecondary)
                {
                    auto& PrimaryQuickBarSlots = Controller->QuickBars->PrimaryQuickBar.Slots;

                    for (int i = 1; i < PrimaryQuickBarSlots.Num(); i++)
                    {
                        if (Params->Pickup->IsActorBeingDestroyed() || Params->Pickup->bPickedUp)
                            return;

                        if (!PrimaryQuickBarSlots[i].Items.Data) // Checks if the slot is empty
                        {
                            if (i >= 6)
                            {
                                auto QuickBars = Controller->QuickBars;

                                auto FocusedSlot = QuickBars->PrimaryQuickBar.CurrentFocusedSlot;

                                if (FocusedSlot == 0) // don't replace the pickaxe
                                    continue;

                                i = FocusedSlot;

                                SDK::FGuid& FocusedGuid = PrimaryQuickBarSlots[FocusedSlot].Items[0];

                                OnDrop(Controller, FocusedGuid, -1);
                            }

                            int Idx = 0;
                            auto entry = AddItemToSlot(Controller, WorldItemDefinition, i, SDK::EFortQuickBars::Primary, Params->Pickup->PrimaryPickupItemEntry.Count, &Idx);

                            auto Instance = GetInstanceFromGuid(Controller, entry.ItemGuid);

                            Instance->ItemEntry.LoadedAmmo = Params->Pickup->PrimaryPickupItemEntry.LoadedAmmo;
                            Instance->ItemEntry.Count = Params->Pickup->PrimaryPickupItemEntry.Count;

                            Update(Controller, i);

                            break;
                        }
                    }
                }

                else
                {
                    auto& SecondaryQuickBarSlots = Controller->QuickBars->SecondaryQuickBar.Slots;

                    for (int i = 0; i < SecondaryQuickBarSlots.Num(); i++)
                    {
                        if (!SecondaryQuickBarSlots[i].Items.Data) // Checks if the slot is empty
                        {
                            AddItemToSlot(Controller, WorldItemDefinition, i, SDK::EFortQuickBars::Secondary, Params->Pickup->PrimaryPickupItemEntry.Count);
                            break;
                        }
                    }
                }
            }

            PickupAnim((SDK::AFortPawn*)Controller->Pawn, Params->Pickup /*, Params->InFlyTime*/);
        }
    }

    static void DumpInventory(SDK::AFortPlayerControllerAthena* PlayerController)
    {
        LOG_INFO("Dumping ({})", PlayerController->PlayerState->GetPlayerName().ToString());
        LOG_INFO("ItemInstances:");

        for (int i = 0; i < PlayerController->WorldInventory->Inventory.ItemInstances.Count; i++)
        {
            auto iteminstance = PlayerController->WorldInventory->Inventory.ItemInstances[i];
            if (!iteminstance)
            {
                LOG_INFO("Item {}: (Empty)", i);
                continue;
            }

            LOG_INFO("Item {}:", i);
            LOG_INFO("    Count: {}", iteminstance->ItemEntry.Count);
            LOG_INFO("    Ammo: {}", iteminstance->ItemEntry.LoadedAmmo);
            LOG_INFO("    ItemDef: {}", iteminstance->ItemEntry.ItemDefinition->GetFullName());
            LOG_INFO("    GUID: ({},{},{},{})", iteminstance->ItemEntry.ItemGuid.A, iteminstance->ItemEntry.ItemGuid.B, iteminstance->ItemEntry.ItemGuid.C, iteminstance->ItemEntry.ItemGuid.D);
        }

        LOG_INFO("PrimaryQuickbar:");
        for (int i = 0; i < PlayerController->QuickBars->PrimaryQuickBar.Slots.Count; i++)
        {
            auto slot = PlayerController->QuickBars->PrimaryQuickBar.Slots[i];

            LOG_INFO("Slot {}:", i, slot.Items.Count);
            LOG_INFO("    Enabled: {}", slot.bEnabled);
            if (slot.Items.Count <= 0)
            {
                LOG_INFO("    Items: (Empty)");
                continue;
            }

            LOG_INFO("    Items:");
            for (int j = 0; j < slot.Items.Count; j++)
            {
                auto item = slot.Items[j];
                LOG_INFO("        {}: ({},{},{},{})", j, item.A, item.B, item.C, item.D);
            }
        }

        LOG_INFO("SecondaryQuickBar:");
        for (int i = 0; i < PlayerController->QuickBars->SecondaryQuickBar.Slots.Count; i++)
        {
            auto slot = PlayerController->QuickBars->SecondaryQuickBar.Slots[i];

            LOG_INFO("Slot {}:", i, slot.Items.Count);
            LOG_INFO("    Enabled: {}", slot.bEnabled);
            if (slot.Items.Count <= 0)
            {
                LOG_INFO("    Items: (Empty)");
                continue;
            }

            LOG_INFO("    Items:");
            for (int j = 0; j < slot.Items.Count; j++)
            {
                auto item = slot.Items[j];
                LOG_INFO("        {}: ({},{},{},{})", j, item.A, item.B, item.C, item.D);
            }
        }
    }

    static void EmptyInventory(SDK::AFortPlayerControllerAthena* PlayerController)
    {
        int i = 0;
        while (i < PlayerController->WorldInventory->Inventory.ItemInstances.Count)
        {
            auto ItemInstance = PlayerController->WorldInventory->Inventory.ItemInstances[i];
            auto ItemDef = ItemInstance->ItemEntry.ItemDefinition;

            if (!ItemDef->IsA(SDK::UFortEditToolItemDefinition::StaticClass()) && !ItemDef->IsA(SDK::UFortWeaponMeleeItemDefinition::StaticClass()) && !ItemDef->IsA(SDK::UFortBuildingItemDefinition::StaticClass()))
            {
                TryDeleteItem(PlayerController, i);
            }
            else
            {
                i++;
            }
        }

        Update(PlayerController, 0, true);
    }

    static void Init(SDK::AFortPlayerControllerAthena* PlayerController)
    {
        PlayerController->QuickBars = Spawners::SpawnActor<SDK::AFortQuickBars>({ -280, 400, 3000 }, PlayerController);
        PlayerController->OnRep_QuickBar();

        // Fixes "Backpack is full" when holding pickaxe
        PlayerController->QuickBars->ServerEnableSlot(SDK::EFortQuickBars::Primary, 0);
        PlayerController->QuickBars->ServerEnableSlot(SDK::EFortQuickBars::Primary, 1);
        PlayerController->QuickBars->ServerEnableSlot(SDK::EFortQuickBars::Primary, 2);
        PlayerController->QuickBars->ServerEnableSlot(SDK::EFortQuickBars::Primary, 3);
        PlayerController->QuickBars->ServerEnableSlot(SDK::EFortQuickBars::Primary, 4);
        PlayerController->QuickBars->ServerEnableSlot(SDK::EFortQuickBars::Primary, 5);
        
        static std::vector<SDK::UFortWorldItemDefinition*> Items = {
            SDK::UObject::FindObject<SDK::UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Wall.BuildingItemData_Wall"),
            SDK::UObject::FindObject<SDK::UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Floor.BuildingItemData_Floor"),
            SDK::UObject::FindObject<SDK::UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Stair_W.BuildingItemData_Stair_W"),
            SDK::UObject::FindObject<SDK::UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_RoofS.BuildingItemData_RoofS"),
            //UObject::FindObject<UFortEditToolItemDefinition>("FortEditToolItemDefinition EditTool.EditTool"),

            SDK::UObject::FindObject<SDK::UFortTrapItemDefinition>("FortTrapItemDefinition TID_Floor_Player_Launch_Pad_Athena.TID_Floor_Player_Launch_Pad_Athena"),
            SDK::UObject::FindObject<SDK::UFortContextTrapItemDefinition>("FortContextTrapItemDefinition TID_ContextTrap_Athena.TID_ContextTrap_Athena"),
            SDK::UObject::FindObject<SDK::UFortTrapItemDefinition>("FortTrapItemDefinition TID_Floor_Player_Campfire_Athena.TID_Floor_Player_Campfire_Athena"),

            SDK::UObject::FindObject<SDK::UFortResourceItemDefinition>("FortResourceItemDefinition WoodItemData.WoodItemData"),
            SDK::UObject::FindObject<SDK::UFortResourceItemDefinition>("FortResourceItemDefinition StoneItemData.StoneItemData"),
            SDK::UObject::FindObject<SDK::UFortResourceItemDefinition>("FortResourceItemDefinition MetalItemData.MetalItemData"),

            SDK::UObject::FindObject<SDK::UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataShells.AthenaAmmoDataShells"),
            SDK::UObject::FindObject<SDK::UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataEnergyCell.AthenaAmmoDataEnergyCell"),
            SDK::UObject::FindObject<SDK::UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium"),
            SDK::UObject::FindObject<SDK::UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight"),
            SDK::UObject::FindObject<SDK::UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy"),
            SDK::UObject::FindObject<SDK::UFortAmmoItemDefinition>("FortAmmoItemDefinition AmmoDataRockets.AmmoDataRockets"),
        };

        int Slot = 0;
        for (auto& Item : Items)
        {
            if (Item->IsA(SDK::UFortAmmoItemDefinition::StaticClass()))
            {
                AddItemToSlot(PlayerController, Item, 0, SDK::EFortQuickBars::Secondary, 100);
                continue;
            }
            else if (Item->IsA(SDK::UFortResourceItemDefinition::StaticClass()))
            {
                AddItemToSlot(PlayerController, Item, 0, SDK::EFortQuickBars::Secondary, 500);
                continue;
            }
            else
            {
                AddItemToSlot(PlayerController, Item, Slot, SDK::EFortQuickBars::Secondary, 1);
            }

            Slot++;
        }

        auto pick = AddItemToSlot(PlayerController, FindWID("WID_Harvest_Pickaxe_Athena_C_T01"), 0);

        static SDK::UFortEditToolItemDefinition* EditTool = SDK::UObject::FindObject<SDK::UFortEditToolItemDefinition>("FortEditToolItemDefinition EditTool.EditTool");
        AddItemToSlot(PlayerController, EditTool, 0, SDK::EFortQuickBars::Primary, 1);
        EquipInventoryItem(PlayerController, pick.ItemGuid);
        PlayerController->SwapQuickBarFocus(SDK::EFortQuickBars::Primary);
        PlayerController->QuickBars->ServerActivateSlotInternal(SDK::EFortQuickBars::Primary, 0, 0, false);
    }
}
