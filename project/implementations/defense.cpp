SoccerCommand BasicPlayer::marcacao()//Faz marcacao
{ 
	ObjectT inimigoMaisProximo = WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,WM->getAgentObjectType(),NULL,-1.0);
	ObjectT amigoMaisProximo = WM->getClosestInSetTo(OBJECT_SET_TEAMMATES,WM->getAgentObjectType(),NULL,-1.0);

	VecPosition posicaoEstrategica = WM->getStrategicPosition();
	VecPosition posInimigoMaisProximo = WM->getGlobalPosition(inimigoMaisProximo);

	Circle circulodeMarcacao = Circle(posInimigoMaisProximo,10);
	Line linha = Line(0,0,0);
	Circle circulo;
	VecPosition intercecao = VecPosition(25,25);
	VecPosition lixo = VecPosition(25,25);

	if(WM->getBallPos().getX() > 30)
		return moveToPos(posicaoEstrategica,PS->getPlayerWhenToTurnAngle());

	if(!WM->isVisible(inimigoMaisProximo))
		return turnBodyToObject(inimigoMaisProximo);

	if(circulodeMarcacao.isInside(WM->getGlobalPosition(amigoMaisProximo)))
	{
		VecPosition posInimigoPosicaoEstrategica =  WM->getClosestInSetTo(OBJECT_SET_OPPONENTS,posicaoEstrategica,NULL,-1.0);
		linha = Line::makeLineFromTwoPoints(posInimigoPosicaoEstrategica,WM->getBallPos());
		circulo = Circle(posInimigoPosicaoEstrategica,5);
		linha.getCircleIntersectionPoints(circulo,&intercecao,&lixo);
		return moveToPos(intercecao,PS->getPlayerWhenToTurnAngle());		
	}
	
	/*if(posInimigoMaisProximo.getDistanceTo(posicaoEstrategica) > 30 && posInimigoMaisProximo.getDistanceTo(WM->getBallPos()) > 40)
		return turnNeckToObject(inimigoMaisProximo,moveToPos(posicaoEstrategica,PS->getPlayerWhenToTurnAngle()));*/	

	linha = Line::makeLineFromTwoPoints(posInimigoMaisProximo,WM->getBallPos());
	circulo = Circle(posInimigoMaisProximo,5);
	linha.getCircleIntersectionPoints(circulo,&intercecao,&lixo);

	return moveToPos(intercecao,PS->getPlayerWhenToTurnAngle());	
}




//Isso vai no PlayerTeams.cpp dentro do bloco "Não estou com a bola"
if(WM->getFastestInSetTo( OBJECT_SET_TEAMMATES, OBJECT_BALL, &iTmp )
                            == WM->getAgentObjectType()  && !WM->isDeadBallThem() )	// if fastest to ball
                    {
                        Log.log( 100, "I am fastest to ball; can get there in %d cycles", iTmp );
                        soc = intercept( false );                      // intercept the ball

                        if( soc.commandType == CMD_DASH &&             // if stamina low
                                WM->getAgentStamina().getStamina() <
                                SS->getRecoverDecThr()*SS->getStaminaMax()+200 )
                        {
                            soc.dPower = 30.0 * WM->getAgentStamina().getRecovery(); // dash slow
                            ACT->putCommandInQueue( soc );
                            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
                        }
                        else                                           // if stamina high
                        {
                            ACT->putCommandInQueue( soc );               // dash as intended
                            ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
                        }	
                    }
                else
				{ 
					if( WM->isBallInOurPossesion())
                	{
                    	if( posAgent.getDistanceTo(WM->getStrategicPosition()) >
                    	        1.5 + fabs(posAgent.getX()-posBall.getX())/10.0)
                                                                      // if not near strategic pos
                    	{
                    	    if( WM->getAgentStamina().getStamina() >     // if stamina high
                    	            SS->getRecoverDecThr()*SS->getStaminaMax()+800 )
                    	    {
                    	        soc = moveToPos(WM->getStrategicPosition(),
                    	                        PS->getPlayerWhenToTurnAngle());
                    	        ACT->putCommandInQueue( soc );            // move to strategic pos
                    	        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
                    	    }
                    	    else                                        // else watch ball
                    	    {
                    	        ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
                    	        ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
                    	    }
                    	}
                    	else if( fabs( WM->getRelativeAngle( OBJECT_BALL ) ) > 1.0 ) // watch ball
                    	{
                    	    ACT->putCommandInQueue( soc = turnBodyToObject( OBJECT_BALL ) );
                    	    ACT->putCommandInQueue( turnNeckToObject( OBJECT_BALL, soc ) );
                    	}
                    	else                                         // nothing to do
                    	    ACT->putCommandInQueue( SoccerCommand(CMD_TURNNECK,0.0) );
                	}	
                	else
                	{
                        soc = marcacao();
                        ACT->putCommandInQueue(soc);
			ACT->putCommandInQueue( alignNeckWithBody( ) );    
                    }
                }
            }
        }
    }
