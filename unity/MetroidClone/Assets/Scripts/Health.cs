﻿using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Health : MonoBehaviour
{
	public int HP
	{
		get { return hp; }
	}

	public Slider healthSlider;

	private Stats stats;
	private int maxHP;
	private int hp;

	void Start ()
	{
		stats = GetComponent<Stats>();
		maxHP = hp = stats.vitality * 10;
		UpdateSlider();
	}

	void UpdateSlider()
	{
		if (healthSlider)
			healthSlider.value = (float)hp / (float)maxHP;
	}

	void OnHit(DamageVector damageVector)
	{
		int damage = (int)(damageVector.physicalDamage - (stats.resistance * 0.2));
		BroadcastMessage("OnDamageTaken", damage);
		hp -= damage;
		UpdateSlider();
		if (hp <= 0)
			Destroy(gameObject);
	}
}
