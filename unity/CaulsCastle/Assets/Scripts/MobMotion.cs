﻿using UnityEngine;
using System.Collections;

[RequireComponent(typeof(Animator))]
[RequireComponent(typeof(Rigidbody2D))]
[RequireComponent(typeof(LockOn))]

public class MobMotion : MonoBehaviour
{
	static int HasTarget = Animator.StringToHash("HasTarget");

	Animator mAnimator;
	Rigidbody2D mRigidbody;
	LockOn mLockOn;

	public Vector2 Movement { get; set; }
	public bool Attack { get; set; }

	void Awake()
	{
		mAnimator = GetComponent<Animator>();
		mRigidbody = GetComponent<Rigidbody2D>();
		mLockOn = GetComponent<LockOn>();
		mLockOn.LockEvent.AddListener(HandleLock);
	}

	void HandleLock()
	{
		mAnimator.SetBool(HasTarget, true);
	}

	void OnAnimatorMove()
	{
		mRigidbody.velocity = mAnimator.deltaPosition / Time.deltaTime;
	}

	void LateUpdate()
	{
		Attack = false;
	}
}
