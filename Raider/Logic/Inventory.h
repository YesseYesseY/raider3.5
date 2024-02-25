#pragma once

#include "../UE4.h"
#include "Spawners.h"

// Pretty sure most of this needs a rework
namespace Inventory
{
    static void Update(AFortPlayerController* Controller, int Dirty = 0, bool bRemovedItem = false)
    {
        Controller->WorldInventory->bRequiresLocalUpdate = true;
        Controller->WorldInventory->HandleInventoryLocalUpdate();
        Controller->WorldInventory->ForceNetUpdate();
        Controller->HandleWorldInventoryLocalUpdate();

        Controller->QuickBars->OnRep_PrimaryQuickBar();
        Controller->QuickBars->OnRep_SecondaryQuickBar();
        Controller->QuickBars->ForceNetUpdate();
        Controller->ForceUpdateQuickbar(EFortQuickBars::Primary);
        Controller->ForceUpdateQuickbar(EFortQuickBars::Secondary);

        if (bRemovedItem)
            Controller->WorldInventory->Inventory.MarkArrayDirty();

        if (Dirty != 0 && Controller->WorldInventory->Inventory.ReplicatedEntries.Num() >= Dirty)
            Controller->WorldInventory->Inventory.MarkItemDirty(Controller->WorldInventory->Inventory.ReplicatedEntries[Dirty]);
    }

    static bool IsValidGuid(AFortPlayerControllerAthena* Controller, const FGuid& Guid)
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

    static FFortItemEntry GetEntryInSlot(AFortPlayerControllerAthena* Controller, int Slot, int Item = 0, EFortQuickBars QuickBars = EFortQuickBars::Primary)
    {
        auto ret = FFortItemEntry();

        auto& ItemInstances = Controller->WorldInventory->Inventory.ItemInstances;

        FGuid ToFindGuid;

        if (QuickBars == EFortQuickBars::Primary)
        {
            ToFindGuid = Controller->QuickBars->PrimaryQuickBar.Slots[Slot].Items[Item];
        }

        else if (QuickBars == EFortQuickBars::Secondary)
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

    inline auto GetDefinitionInSlot(AFortPlayerControllerAthena* Controller, int Slot, int Item = 0, EFortQuickBars QuickBars = EFortQuickBars::Primary)
    {
        return GetEntryInSlot(Controller, Slot, Item, QuickBars).ItemDefinition;
    }

    static UFortWorldItem* GetInstanceFromGuid(AController* PC, const FGuid& ToFindGuid)
    {
        auto& ItemInstances = static_cast<AFortPlayerController*>(PC)->WorldInventory->Inventory.ItemInstances;

        for (int j = 0; j < ItemInstances.Num(); j++)
        {
            auto ItemInstance = ItemInstances[j];

            if (!ItemInstance)
                continue;

            auto Guid = ItemInstance->ItemEntry.ItemGuid;

            if (ToFindGuid == Guid)
            {
                return ItemInstance;
            }
        }

        return nullptr;
    }

    static auto AddItemToSlot(AFortPlayerControllerAthena* Controller, UFortWorldItemDefinition* Definition, int Slot, EFortQuickBars Bars = EFortQuickBars::Primary, int Count = 1, int* Idx = nullptr)
    {
        auto ret = FFortItemEntry();

        if (Definition->IsA(UFortWeaponItemDefinition::StaticClass()))
        {
            Count = 1;
        }

        if (Slot < 0)
        {
            Slot = 1;
        }

        if (Bars == EFortQuickBars::Primary && Slot >= 6)
        {
            Slot = 5;
        }

        if (auto TempItemInstance = static_cast<UFortWorldItem*>(Definition->CreateTemporaryItemInstanceBP(Count, 1)))
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

    static bool RemoveItemFromSlot(AFortPlayerControllerAthena* Controller, int Slot, EFortQuickBars Quickbars = EFortQuickBars::Primary, int Amount = -1) // -1 for all items in the slot
    {
        auto& PrimarySlots = Controller->QuickBars->PrimaryQuickBar.Slots;
        auto& SecondarySlots = Controller->QuickBars->SecondaryQuickBar.Slots;

        bool bPrimaryQuickBar = (Quickbars == EFortQuickBars::Primary);

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

    static bool IsGuidInInventory(AFortPlayerControllerAthena* Controller, const FGuid& Guid)
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

    static AFortWeapon* EquipWeaponDefinition(APawn* dPawn, UFortWeaponItemDefinition* Definition, const FGuid& Guid, int Ammo = -1, bool bEquipWithMaxAmmo = false) // don't use, use EquipInventoryItem // not too secure
    {
        AFortWeapon* Weapon = nullptr;

        auto weaponClass = Definition->GetWeaponActorClass();
        auto Pawn = static_cast<APlayerPawn_Athena_C*>(dPawn);

        if (Pawn && Definition && weaponClass)
        {
            auto Controller = static_cast<AFortPlayerControllerAthena*>(Pawn->Controller);
            auto Instance = GetInstanceFromGuid(Controller, Guid);

            if (!Inventory::IsGuidInInventory(Controller, Guid))
                return Weapon;

            if (weaponClass == ATrapTool_C::StaticClass() || weaponClass == ATrapTool_ContextTrap_Athena_C::StaticClass() || weaponClass == AFortDecoTool_ContextTrap::StaticClass())
            {
                Weapon = static_cast<AFortWeapon*>(Spawners::SpawnActor(weaponClass, {}, Pawn)); // Other people can't see their weapon.

                if (Weapon)
                {
                    Weapon->bReplicates = true;
                    Weapon->bOnlyRelevantToOwner = false;
                    
                    if (Definition->IsA(UFortContextTrapItemDefinition::StaticClass()))
                    {
                        static_cast<AFortDecoTool_ContextTrap*>(Weapon)->ContextTrapItemDefinition = (UFortContextTrapItemDefinition*)Definition;
                    }
                    static_cast<AFortTrapTool*>(Weapon)->ItemDefinition = Definition;
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

    static void EquipInventoryItem(AFortPlayerControllerAthena* PC, FGuid& Guid)
    {
        if (!PC || PC->IsInAircraft())
            return;

        auto& ItemInstances = PC->WorldInventory->Inventory.ItemInstances;

        for (int i = 0; i < ItemInstances.Num(); i++)
        {
            auto CurrentItemInstance = ItemInstances[i];

            if (!CurrentItemInstance)
                continue;

            auto Def = static_cast<UFortWeaponItemDefinition*>(CurrentItemInstance->GetItemDefinitionBP());

            if (CurrentItemInstance->GetItemGuid() == Guid && Def)
            {
                EquipWeaponDefinition(PC->Pawn, Def, Guid); // CurrentItemInstance->ItemEntry.LoadedAmmo);
                break;
            }
        }
    }

    void EquipLoadout(AFortPlayerControllerAthena* Controller, PlayerLoadout WIDS)
    {
        FFortItemEntry pickaxeEntry;

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

    static bool OnDrop(AFortPlayerControllerAthena* Controller, void* params)
    {
        auto Params = static_cast<AFortPlayerController_ServerAttemptInventoryDrop_Params*>(params);

        if (!Params || !Controller)
            return false;

        auto& ItemInstances = Controller->WorldInventory->Inventory.ItemInstances;
        auto QuickBars = Controller->QuickBars;

        auto& PrimaryQuickBarSlots = QuickBars->PrimaryQuickBar.Slots;
        auto& SecondaryQuickBarSlots = QuickBars->SecondaryQuickBar.Slots;

        bool bWasSuccessful = false;

        for (int i = 1; i < PrimaryQuickBarSlots.Num(); i++)
        {
            if (PrimaryQuickBarSlots[i].Items.Data)
            {
                for (int j = 0; j < PrimaryQuickBarSlots[i].Items.Num(); j++)
                {
                    if (PrimaryQuickBarSlots[i].Items[j] == Params->ItemGuid)
                    {
                        auto Instance = GetInstanceFromGuid(Controller, Params->ItemGuid);

                        if (Instance)
                        {
                            auto Definition = Instance->ItemEntry.ItemDefinition;
                            auto SuccessfullyRemoved = RemoveItemFromSlot(Controller, i, EFortQuickBars::Primary, j + 1);

                            if (Definition && SuccessfullyRemoved)
                            {
                                auto Pickup = Spawners::SummonPickup(static_cast<AFortPlayerPawn*>(Controller->Pawn), Definition, 1, Controller->Pawn->K2_GetActorLocation());
                                Pickup->PrimaryPickupItemEntry.LoadedAmmo = Instance->GetLoadedAmmo();
                                bWasSuccessful = true;
                                break;
                            }

                            LOG_ERROR("Couldn't find a definition for the dropped item!");
                        }
                    }
                }
            }
        }

        if (!bWasSuccessful)
        {
            for (int i = 0; i < SecondaryQuickBarSlots.Num(); i++)
            {
                if (SecondaryQuickBarSlots[i].Items.Data)
                {
                    for (int j = 0; j < SecondaryQuickBarSlots[i].Items.Num(); j++)
                    {
                        if (SecondaryQuickBarSlots[i].Items[j] == Params->ItemGuid)
                        {
                            auto Definition = GetDefinitionInSlot(Controller, i, j, EFortQuickBars::Secondary);
                            auto bSucceeded = RemoveItemFromSlot(Controller, i, EFortQuickBars::Secondary, j + 1);

                            if (Definition && bSucceeded)
                            {
                                Spawners::SummonPickup(static_cast<AFortPlayerPawn*>(Controller->Pawn), Definition, 1, Controller->Pawn->K2_GetActorLocation());
                                bWasSuccessful = true;
                                break;
                            }
                            LOG_ERROR("Couldn't find a definition for the dropped item!");
                        }
                    }
                }
            }
        }

        if (bWasSuccessful && PrimaryQuickBarSlots[0].Items.Data)
            EquipInventoryItem(Controller, PrimaryQuickBarSlots[0].Items[0]); // just select pickaxe for now

        return bWasSuccessful;
    }

    inline void PickupAnim(AFortPawn* Pawn, AFortPickup* Pickup, float FlyTime = 0.40f)
    {
        Pickup->ForceNetUpdate();
        Pickup->PickupLocationData.PickupTarget = Pawn;
        Pickup->PickupLocationData.FlyTime = FlyTime;
        Pickup->PickupLocationData.ItemOwner = Pawn;
        Pickup->OnRep_PickupLocationData();

        Pickup->bPickedUp = true;
        Pickup->OnRep_bPickedUp();
    }

    static bool CanPickup(AFortPlayerControllerAthena* Controller, AFortPickup* Pickup)
    {
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

    inline void OnPickup(AFortPlayerControllerAthena* Controller, void* params)
    {
        auto Params = static_cast<AFortPlayerPawn_ServerHandlePickup_Params*>(params);

        if (!Controller || !Params)
            return;

        auto& ItemInstances = Controller->WorldInventory->Inventory.ItemInstances;


        if (Params->Pickup)
        {
            bool bCanGoInSecondary = true; // there is no way this is how you do it // todo: rename

            if (Params->Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortWeaponItemDefinition::StaticClass()) && !Params->Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass()))
                bCanGoInSecondary = false;

            auto WorldItemDefinition = static_cast<UFortWorldItemDefinition*>(Params->Pickup->PrimaryPickupItemEntry.ItemDefinition);

            int DupItemIndex = -1;
            for (int i = 0; i < ItemInstances.Count; i++)
            {
                if (ItemInstances[i]->ItemEntry.ItemDefinition == WorldItemDefinition)
                {
                    if (ItemInstances[i]->ItemEntry.Count >= WorldItemDefinition->MaxStackSize)
                    {
                        if (WorldItemDefinition->bAllowMultipleStacks)
                            continue;
                    }

                    DupItemIndex = i;
                    break;
                }
            }
            LOG_INFO("DupItemIndex: {}", DupItemIndex);

            if (DupItemIndex != -1)
            {
                auto newcount = ItemInstances[DupItemIndex]->ItemEntry.Count + Params->Pickup->PrimaryPickupItemEntry.Count;
                if (newcount > WorldItemDefinition->MaxStackSize)
                {
                    auto leftover = newcount - WorldItemDefinition->MaxStackSize;
                    Spawners::SummonPickup(static_cast<APlayerPawn_Athena_C*>(Controller->Pawn), WorldItemDefinition, leftover, Controller->Pawn->K2_GetActorLocation());
                    newcount = WorldItemDefinition->MaxStackSize;
                }

                Controller->WorldInventory->Inventory.ItemInstances[DupItemIndex]->ItemEntry.Count = newcount;
                Controller->WorldInventory->Inventory.ReplicatedEntries[DupItemIndex].Count = newcount;

                Update(Controller, DupItemIndex);
            }
            else
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

                                FGuid& FocusedGuid = PrimaryQuickBarSlots[FocusedSlot].Items[0];

                                auto Instance = GetInstanceFromGuid(Controller, FocusedGuid);

                                // if (Params->Pickup->MultiItemPickupEntries)
                                auto pickup = Spawners::SummonPickup(static_cast<APlayerPawn_Athena_C*>(Controller->Pawn), Instance->ItemEntry.ItemDefinition, Instance->ItemEntry.Count, Controller->Pawn->K2_GetActorLocation());
                                pickup->PrimaryPickupItemEntry.LoadedAmmo = Instance->GetLoadedAmmo();

                                RemoveItemFromSlot(Controller, FocusedSlot, EFortQuickBars::Primary);
                            }

                            int Idx = 0;
                            auto entry = AddItemToSlot(Controller, WorldItemDefinition, i, EFortQuickBars::Primary, Params->Pickup->PrimaryPickupItemEntry.Count, &Idx);
                            // auto& Entry = Controller->WorldInventory->Inventory.ReplicatedEntries[Idx];

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

                    // else
                    //{
                    for (int i = 0; i < SecondaryQuickBarSlots.Num(); i++)
                    {
                        if (!SecondaryQuickBarSlots[i].Items.Data) // Checks if the slot is empty
                        {
                            AddItemToSlot(Controller, WorldItemDefinition, i, EFortQuickBars::Secondary, Params->Pickup->PrimaryPickupItemEntry.Count);
                            break;
                        }
                    }
                    //}
                }
            }


            PickupAnim((AFortPawn*)Controller->Pawn, Params->Pickup /*, Params->InFlyTime*/);
        }
    }

    template <typename Class>
    static FFortItemEntry FindItemInInventory(AFortPlayerControllerAthena* PC, bool& bFound)
    {
        auto ret = FFortItemEntry();

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

    static bool TryDeleteItem(AFortPlayerControllerAthena* PlayerController, int Index)
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
            PlayerController->QuickBars->EmptySlot(primary ? EFortQuickBars::Primary : EFortQuickBars::Secondary, slot);
            if (primary)
                PlayerController->QuickBars->PrimaryQuickBar.Slots[slot].Items.FreeArray();
            else
                PlayerController->QuickBars->SecondaryQuickBar.Slots[slot].Items.FreeArray();
        }
        else
        {
            LOG_WARN("Quickbar slot not found in TryDeleteItem");
        }
        
        Update(PlayerController, Index, true);

        return true;
    }

    static bool TryRemoveItem(AFortPlayerControllerAthena* PlayerController, UFortItemDefinition* ItemDef, int AmountToRemove)
    {
        auto& ItemInstances = PlayerController->WorldInventory->Inventory.ItemInstances;

        if (!ItemDef)
            return false;
        bool success = false;
        for (int i = 0; i < ItemInstances.Num(); i++)
        {
            auto ItemInstance = ItemInstances[i];

            if (!ItemInstance)
                continue;

            if (ItemInstance->ItemEntry.ItemDefinition && ItemInstance->ItemEntry.ItemDefinition == ItemDef)
            {
                auto finalcount = ItemInstance->ItemEntry.Count - AmountToRemove;
                if (finalcount > 0)
                {
                    ItemInstance->ItemEntry.Count = finalcount;
                    PlayerController->WorldInventory->Inventory.ReplicatedEntries[i].Count = finalcount;
                }
                else
                {
                    if (!TryDeleteItem(PlayerController, i))
                    {
                        LOG_ERROR("Failed to remove item entry {}", i);
                    }
                }
                Update(PlayerController, i);
                success = true;
                break;
            }
        }

        return success;
    }

    static void DumpInventory(AFortPlayerControllerAthena* PlayerController)
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

    static void EmptyInventory(AFortPlayerControllerAthena* PlayerController)
    {
        int i = 0;
        while (i < PlayerController->WorldInventory->Inventory.ItemInstances.Count)
        {
            auto ItemInstance = PlayerController->WorldInventory->Inventory.ItemInstances[i];
            auto ItemDef = ItemInstance->ItemEntry.ItemDefinition;

            if (!ItemDef->IsA(UFortEditToolItemDefinition::StaticClass()) && !ItemDef->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) && !ItemDef->IsA(UFortBuildingItemDefinition::StaticClass()))
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

    static void Init(AFortPlayerControllerAthena* PlayerController)
    {
        PlayerController->QuickBars = Spawners::SpawnActor<AFortQuickBars>({ -280, 400, 3000 }, PlayerController);
        PlayerController->OnRep_QuickBar();

        // Fixes "Backpack is full" when holding pickaxe
        PlayerController->QuickBars->ServerEnableSlot(EFortQuickBars::Primary, 0);
        PlayerController->QuickBars->ServerEnableSlot(EFortQuickBars::Primary, 1);
        PlayerController->QuickBars->ServerEnableSlot(EFortQuickBars::Primary, 2);
        PlayerController->QuickBars->ServerEnableSlot(EFortQuickBars::Primary, 3);
        PlayerController->QuickBars->ServerEnableSlot(EFortQuickBars::Primary, 4);
        PlayerController->QuickBars->ServerEnableSlot(EFortQuickBars::Primary, 5);
        
        static std::vector<UFortWorldItemDefinition*> Items = {
            UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Wall.BuildingItemData_Wall"),
            UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Floor.BuildingItemData_Floor"),
            UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_Stair_W.BuildingItemData_Stair_W"),
            UObject::FindObject<UFortBuildingItemDefinition>("FortBuildingItemDefinition BuildingItemData_RoofS.BuildingItemData_RoofS"),
            //UObject::FindObject<UFortEditToolItemDefinition>("FortEditToolItemDefinition EditTool.EditTool"),

            UObject::FindObject<UFortTrapItemDefinition>("FortTrapItemDefinition TID_Floor_Player_Launch_Pad_Athena.TID_Floor_Player_Launch_Pad_Athena"),
            UObject::FindObject<UFortContextTrapItemDefinition>("FortContextTrapItemDefinition TID_ContextTrap_Athena.TID_ContextTrap_Athena"),
            UObject::FindObject<UFortTrapItemDefinition>("FortTrapItemDefinition TID_Floor_Player_Campfire_Athena.TID_Floor_Player_Campfire_Athena"),

            UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition WoodItemData.WoodItemData"),
            UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition StoneItemData.StoneItemData"),
            UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition MetalItemData.MetalItemData"),

            UObject::FindObject<UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataShells.AthenaAmmoDataShells"),
            UObject::FindObject<UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataEnergyCell.AthenaAmmoDataEnergyCell"),
            UObject::FindObject<UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium"),
            UObject::FindObject<UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight"),
            UObject::FindObject<UFortAmmoItemDefinition>("FortAmmoItemDefinition AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy"),
            UObject::FindObject<UFortAmmoItemDefinition>("FortAmmoItemDefinition AmmoDataRockets.AmmoDataRockets"),
        };

        int Slot = 0;
        for (auto& Item : Items)
        {
            if (Item->IsA(UFortAmmoItemDefinition::StaticClass()))
            {
                AddItemToSlot(PlayerController, Item, 0, EFortQuickBars::Secondary, 100);
                continue;
            }
            else if (Item->IsA(UFortResourceItemDefinition::StaticClass()))
            {
                AddItemToSlot(PlayerController, Item, 0, EFortQuickBars::Secondary, 500);
                continue;
            }
            else
            {
                AddItemToSlot(PlayerController, Item, Slot, EFortQuickBars::Secondary, 1);
            }

            Slot++;
        }

        auto pick = AddItemToSlot(PlayerController, FindWID("WID_Harvest_Pickaxe_Athena_C_T01"), 0);

        static UFortEditToolItemDefinition* EditTool = UObject::FindObject<UFortEditToolItemDefinition>("FortEditToolItemDefinition EditTool.EditTool");
        AddItemToSlot(PlayerController, EditTool, 0, EFortQuickBars::Primary, 1);
        EquipInventoryItem(PlayerController, pick.ItemGuid);

        PlayerController->QuickBars->ServerActivateSlotInternal(EFortQuickBars::Primary, 0, 0, false);
    }
}
