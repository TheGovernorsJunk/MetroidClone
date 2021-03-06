﻿using UnityEngine;
using System.Collections;

[RequireComponent(typeof(Animator))]
[RequireComponent(typeof(Rigidbody2D))]
[RequireComponent(typeof(LockOn))]

public class MobMotion : MonoBehaviour
{
	static int HasTarget = Animator.StringToHash("HasTarget");
	static int NormalX = Animator.StringToHash("NormalX");
	static int NormalY = Animator.StringToHash("NormalY");
	static int MovX = Animator.StringToHash("MovX");
	static int MovY = Animator.StringToHash("MovY");

	Animator mAnimator;
	Rigidbody2D mRigidbody;
	LockOn mLockOn;

	public Vector2 Heading
	{
		get
		{
			return new Vector2(mAnimator.GetFloat(NormalX), mAnimator.GetFloat(NormalY));
		}
	}

	public Vector2 Movement
	{
		get
		{
			return new Vector2(mAnimator.GetFloat(MovX), mAnimator.GetFloat(MovY));
		}
	}

	//public Vector2 Movement { get; set; }
	public bool PendingAttack { get; set; }
	public bool PendingDodge { get; set; }
	public Vector2 PendingMovement { get; set; }

	void Awake()
	{
		mAnimator = GetComponent<Animator>();
		mRigidbody = GetComponent<Rigidbody2D>();
		mLockOn = GetComponent<LockOn>();
	}

	void Start()
	{
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
		PendingAttack = false;
		PendingDodge = false;
	}
}
