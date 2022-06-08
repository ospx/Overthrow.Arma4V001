class OVT_BaseUpgradeTownPatrol : OVT_BasePatrolUpgrade
{
	[Attribute("2000", desc: "Max range of towns to patrol")]
	float m_fRange;
	
	protected ref array<ref OVT_TownData> m_TownsInRange;
	protected ref map<EntityID, EntityID> m_Patrols;
		
	override void PostInit()
	{
		super.PostInit();
		
		m_TownsInRange = new array<ref OVT_TownData>;
		m_Patrols = new map<EntityID, EntityID>;
		
		OVT_TownManagerComponent.GetInstance().GetTownsWithinDistance(m_BaseController.GetOwner().GetOrigin(), m_fRange, m_TownsInRange);
	}
	
	override int Spend(int resources)
	{
		int spent = 0;
		
		foreach(OVT_TownData town : m_TownsInRange)
		{
			if(resources <= 0) break;
			if(!m_Patrols.Contains(town.markerID))
			{
				int newres = BuyPatrol(town);
				
				spent += newres;
				resources -= newres;
			}else{
				//Check if theyre back
				SCR_AIGroup aigroup = GetGroup(m_Patrols[town.markerID]);
				float distance = vector.Distance(aigroup.GetOrigin(), m_BaseController.GetOwner().GetOrigin());
				int agentCount = aigroup.GetAgentsCount();
				if(distance < 20 || agentCount == 0)
				{
					//Recover any resources
					m_occupyingFactionManager.RecoverResources(agentCount * m_Config.m_Difficulty.resourcesPerSoldier);
					
					m_Patrols.Remove(town.markerID);
					SCR_Global.DeleteEntityAndChildren(aigroup);	
					
					//send another one
					int newres = BuyPatrol(town);
				
					spent += newres;
					resources -= newres;			
				}
			}
		}
		
		return spent;
	}
	
	protected int BuyPatrol(OVT_TownData town)
	{
		#ifdef OVERTHROW_DEBUG
		Print("Sending patrol to " + town.name);
		#endif
		
		OVT_Faction faction = m_Config.GetOccupyingFaction();
		BaseWorld world = GetGame().GetWorld();
		
		ResourceName res = faction.m_aGroupInfantryPrefabSlots.GetRandomElement();
			
		EntitySpawnParams spawnParams = new EntitySpawnParams;
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		vector pos = m_BaseController.GetOwner().GetOrigin();
		
		float surfaceY = world.GetSurfaceY(pos[0], pos[2]);
		if (pos[1] < surfaceY)
		{
			pos[1] = surfaceY;
		}
		
		spawnParams.Transform[3] = pos;
		IEntity group = GetGame().SpawnEntityPrefab(Resource.Load(res), world, spawnParams);
		
		m_Groups.Insert(group.GetID());
		
		SCR_AIGroup aigroup = SCR_AIGroup.Cast(group);
		
		AddWaypoints(aigroup, town);
		m_Patrols[town.markerID] = aigroup.GetID();
		
		int newres = aigroup.m_aUnitPrefabSlots.Count() * m_Config.m_Difficulty.resourcesPerSoldier;
			
		return newres;
	}
	
	protected void AddWaypoints(SCR_AIGroup aigroup, OVT_TownData town)
	{
		IEntity marker = GetGame().GetWorld().FindEntityByID(town.markerID);
		
		array<AIWaypoint> queueOfWaypoints = new array<AIWaypoint>();
		
		if(m_BaseController.m_AllCloseSlots.Count() > 2)
		{						
			aigroup.AddWaypoint(SpawnPatrolWaypoint(marker.GetOrigin()));
			
			aigroup.AddWaypoint(SpawnWaitWaypoint(marker.GetOrigin(), s_AIRandomGenerator.RandFloatXY(45, 75)));								
			
			aigroup.AddWaypoint(SpawnPatrolWaypoint(m_BaseController.GetOwner().GetOrigin()));
			
		}
	}
}